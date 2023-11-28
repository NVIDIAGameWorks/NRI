/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "SharedExternal.h"
#include "DeviceBase.h"
#include "DeviceVal.h"
#include "SharedVal.h"
#include "CommandBufferVal.h"

#include "BufferVal.h"
#include "DescriptorVal.h"
#include "DescriptorSetVal.h"
#include "DescriptorPoolVal.h"
#include "FrameBufferVal.h"
#include "PipelineLayoutVal.h"
#include "PipelineVal.h"
#include "QueryPoolVal.h"
#include "TextureVal.h"
#include "AccelerationStructureVal.h"

using namespace nri;

void ConvertGeometryObjectsVal(GeometryObject* destObjects, const GeometryObject* sourceObjects, uint32_t objectNum);

static bool ValidateBufferTransitionBarrierDesc(const DeviceVal& device, uint32_t i, const BufferTransitionBarrierDesc& bufferTransitionBarrierDesc);
static bool ValidateTextureTransitionBarrierDesc(const DeviceVal& device, uint32_t i, const TextureTransitionBarrierDesc& textureTransitionBarrierDesc);

CommandBufferVal::CommandBufferVal(DeviceVal& device, CommandBuffer& commandBuffer, bool isWrapped) :
    DeviceObjectVal(device, commandBuffer),
    m_ValidationCommands(device.GetStdAllocator()),
    m_RayTracingAPI(device.GetRayTracingInterface()),
    m_MeshShaderAPI(device.GetMeshShaderInterface()),
    m_IsWrapped(isWrapped),
    m_IsRecordingStarted(isWrapped)
{
}

void CommandBufferVal::SetDebugName(const char* name)
{
    m_Name = name;
    GetCoreInterface().SetCommandBufferDebugName(GetImpl(), name);
}

Result CommandBufferVal::Begin(const DescriptorPool* descriptorPool, uint32_t nodeIndex)
{
    RETURN_ON_FAILURE(&m_Device, !m_IsRecordingStarted, Result::FAILURE,
        "Can't begin recording of CommandBuffer: the command buffer is already in the recording state.");

    DescriptorPool* descriptorPoolImpl = nullptr;
    if (descriptorPool)
        descriptorPoolImpl = NRI_GET_IMPL_PTR(DescriptorPool, descriptorPool);

    Result result = GetCoreInterface().BeginCommandBuffer(GetImpl(), descriptorPoolImpl, nodeIndex);
    if (result == Result::SUCCESS)
        m_IsRecordingStarted = true;

    m_ValidationCommands.clear();

    return result;
}

Result CommandBufferVal::End()
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, Result::FAILURE,
        "Can't end command buffer: the command buffer must be in the recording state.");

    if (m_AnnotationStack > 0)
        REPORT_ERROR(&m_Device, "BeginAnnotation() is called more times than EndAnnotation()");
    else if (m_AnnotationStack < 0)
        REPORT_ERROR(&m_Device, "EndAnnotation() is called more times than BeginAnnotation()");

    Result result = GetCoreInterface().EndCommandBuffer(GetImpl());

    if (result == Result::SUCCESS)
    {
        m_IsRecordingStarted = m_IsWrapped;
        m_FrameBuffer = nullptr;
    }

    return result;
}

void CommandBufferVal::SetViewports(const Viewport* viewports, uint32_t viewportNum)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't set viewports: the command buffer must be in the recording state.");

    if (viewportNum == 0)
        return;

    RETURN_ON_FAILURE(&m_Device, viewports != nullptr, ReturnVoid(),
        "Can't set viewports: 'viewports' is invalid.");

    GetCoreInterface().CmdSetViewports(GetImpl(), viewports, viewportNum);
}

void CommandBufferVal::SetScissors(const Rect* rects, uint32_t rectNum)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't set scissors: the command buffer must be in the recording state.");

    if (rectNum == 0)
        return;

    RETURN_ON_FAILURE(&m_Device, rects != nullptr, ReturnVoid(),
        "Can't set scissor rects: 'rects' is invalid.");

    GetCoreInterface().CmdSetScissors(GetImpl(), rects, rectNum);
}

void CommandBufferVal::SetDepthBounds(float boundsMin, float boundsMax)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't set depth bounds: the command buffer must be in the recording state.");

    GetCoreInterface().CmdSetDepthBounds(GetImpl(), boundsMin, boundsMax);
}

void CommandBufferVal::SetStencilReference(uint8_t reference)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't set stencil reference: the command buffer must be in the recording state.");

    GetCoreInterface().CmdSetStencilReference(GetImpl(), reference);
}

void CommandBufferVal::SetSamplePositions(const SamplePosition* positions, uint32_t positionNum)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't set sample positions: the command buffer must be in the recording state.");

    GetCoreInterface().CmdSetSamplePositions(GetImpl(), positions, positionNum);
}

void CommandBufferVal::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't clear attachments: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer != nullptr, ReturnVoid(),
        "Can't clear attachments: no FrameBuffer bound.");

    GetCoreInterface().CmdClearAttachments(GetImpl(), clearDescs, clearDescNum, rects, rectNum);
}

void CommandBufferVal::ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't clear storage buffer: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't clear storage buffer: this operation is not allowed in render pass.");

    RETURN_ON_FAILURE(&m_Device, clearDesc.storageBuffer != nullptr, ReturnVoid(),
        "Can't clear storage buffer: 'clearDesc.storageBuffer' is invalid.");

    auto clearDescImpl = clearDesc;
    clearDescImpl.storageBuffer = NRI_GET_IMPL_PTR(Descriptor, clearDesc.storageBuffer);

    GetCoreInterface().CmdClearStorageBuffer(GetImpl(), clearDescImpl);
}

void CommandBufferVal::ClearStorageTexture(const ClearStorageTextureDesc& clearDesc)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't clear storage texture: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't clear storage texture: this operation is not allowed in render pass.");

    RETURN_ON_FAILURE(&m_Device, clearDesc.storageTexture != nullptr, ReturnVoid(),
        "Can't clear storage texture: 'clearDesc.storageTexture' is invalid.");

    auto clearDescImpl = clearDesc;
    clearDescImpl.storageTexture = NRI_GET_IMPL_PTR(Descriptor, clearDesc.storageTexture);

    GetCoreInterface().CmdClearStorageTexture(GetImpl(), clearDescImpl);
}

void CommandBufferVal::BeginRenderPass(const FrameBuffer& frameBuffer, RenderPassBeginFlag renderPassBeginFlag)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't begin render pass: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't begin render pass: render pass already started.");

    RETURN_ON_FAILURE(&m_Device, renderPassBeginFlag < RenderPassBeginFlag::MAX_NUM, ReturnVoid(),
        "Can't begin render pass: 'renderPassBeginFlag' is invalid.");

    m_FrameBuffer = &frameBuffer;

    FrameBuffer* frameBufferImpl = NRI_GET_IMPL_REF(FrameBuffer, &frameBuffer);

    GetCoreInterface().CmdBeginRenderPass(GetImpl(), *frameBufferImpl, renderPassBeginFlag);
}

void CommandBufferVal::EndRenderPass()
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't end render pass: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer != nullptr, ReturnVoid(),
        "Can't end render pass: no render pass.");

    m_FrameBuffer = nullptr;

    GetCoreInterface().CmdEndRenderPass(GetImpl());
}

void CommandBufferVal::SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't set vertex buffers: the command buffer must be in the recording state.");

    Buffer** buffersImpl = STACK_ALLOC(Buffer*, bufferNum);
    for (uint32_t i = 0; i < bufferNum; i++)
        buffersImpl[i] = NRI_GET_IMPL_PTR(Buffer, buffers[i]);

    GetCoreInterface().CmdSetVertexBuffers(GetImpl(), baseSlot, bufferNum, buffersImpl, offsets);
}

void CommandBufferVal::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't set index buffers: the command buffer must be in the recording state.");

    Buffer* bufferImpl = NRI_GET_IMPL_REF(Buffer, &buffer);

    GetCoreInterface().CmdSetIndexBuffer(GetImpl(), *bufferImpl, offset, indexType);
}

void CommandBufferVal::SetPipelineLayout(const PipelineLayout& pipelineLayout)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't set pipeline layout: the command buffer must be in the recording state.");

    PipelineLayout* pipelineLayoutImpl = NRI_GET_IMPL_REF(PipelineLayout, &pipelineLayout);

    GetCoreInterface().CmdSetPipelineLayout(GetImpl(), *pipelineLayoutImpl);
}

void CommandBufferVal::SetPipeline(const Pipeline& pipeline)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't set pipeline: the command buffer must be in the recording state.");

    Pipeline* pipelineImpl = NRI_GET_IMPL_REF(Pipeline, &pipeline);

    GetCoreInterface().CmdSetPipeline(GetImpl(), *pipelineImpl);
}

void CommandBufferVal::SetDescriptorPool(const DescriptorPool& descriptorPool)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't set descriptor pool: the command buffer must be in the recording state.");

    DescriptorPool* descriptorPoolImpl = NRI_GET_IMPL_REF(DescriptorPool, &descriptorPool);

    GetCoreInterface().CmdSetDescriptorPool(GetImpl(), *descriptorPoolImpl);
}

void CommandBufferVal::SetDescriptorSet(uint32_t setIndexInPipelineLayout, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't set descriptor sets: the command buffer must be in the recording state.");

    DescriptorSet* descriptorSetImpl = NRI_GET_IMPL_REF(DescriptorSet, &descriptorSet);

    GetCoreInterface().CmdSetDescriptorSet(GetImpl(), setIndexInPipelineLayout, *descriptorSetImpl, dynamicConstantBufferOffsets);
}

void CommandBufferVal::SetConstants(uint32_t pushConstantIndex, const void* data, uint32_t size)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't set constants: the command buffer must be in the recording state.");

    GetCoreInterface().CmdSetConstants(GetImpl(), pushConstantIndex, data, size);
}

void CommandBufferVal::Draw(uint32_t vertexNum, uint32_t instanceNum, uint32_t baseVertex, uint32_t baseInstance)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't record draw call: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer != nullptr, ReturnVoid(),
        "Can't record draw call: this operation is allowed only inside render pass.");

    GetCoreInterface().CmdDraw(GetImpl(), vertexNum, instanceNum, baseVertex, baseInstance);
}

void CommandBufferVal::DrawIndexed(uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't record draw call: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer != nullptr, ReturnVoid(),
        "Can't record draw call: this operation is allowed only inside render pass.");

    GetCoreInterface().CmdDrawIndexed(GetImpl(), indexNum, instanceNum, baseIndex, baseVertex, baseInstance);
}

void CommandBufferVal::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't record draw call: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer != nullptr, ReturnVoid(),
        "Can't record draw call: this operation is allowed only inside render pass.");

    Buffer* bufferImpl = NRI_GET_IMPL_REF(Buffer, &buffer);

    GetCoreInterface().CmdDrawIndirect(GetImpl(), *bufferImpl, offset, drawNum, stride);
}

void CommandBufferVal::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't record draw call: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer != nullptr, ReturnVoid(),
        "Can't record draw call: this operation is allowed only inside render pass.");

    Buffer* bufferImpl = NRI_GET_IMPL_REF(Buffer, &buffer);

    GetCoreInterface().CmdDrawIndexedIndirect(GetImpl(), *bufferImpl, offset, drawNum, stride);
}

void CommandBufferVal::CopyBuffer(Buffer& dstBuffer, uint32_t dstNodeIndex, uint64_t dstOffset, const Buffer& srcBuffer,
    uint32_t srcNodeIndex, uint64_t srcOffset, uint64_t size)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't copy buffer: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't copy buffer: this operation is allowed only outside render pass.");

    if (size == WHOLE_SIZE)
    {
        const BufferDesc& dstDesc = ((BufferVal&)dstBuffer).GetDesc();
        const BufferDesc& srcDesc = ((BufferVal&)srcBuffer).GetDesc();

        if (dstDesc.size != srcDesc.size)
        {
            REPORT_WARNING(&m_Device, "WHOLE_SIZE is used but 'dstBuffer' and 'srcBuffer' have diffenet sizes. "
                "'srcDesc.size' bytes will be copied to the destination.");
            return;
        }
    }

    Buffer* dstBufferImpl = NRI_GET_IMPL_REF(Buffer, &dstBuffer);
    Buffer* srcBufferImpl = NRI_GET_IMPL_REF(Buffer, &srcBuffer);

    GetCoreInterface().CmdCopyBuffer(GetImpl(), *dstBufferImpl, dstNodeIndex, dstOffset, *srcBufferImpl, srcNodeIndex,
        srcOffset, size);
}

void CommandBufferVal::CopyTexture(Texture& dstTexture, uint32_t dstNodeIndex, const TextureRegionDesc* dstRegionDesc,
    const Texture& srcTexture, uint32_t srcNodeIndex, const TextureRegionDesc* srcRegionDesc)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't copy texture: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't copy texture: this operation is allowed only outside render pass.");

    RETURN_ON_FAILURE(&m_Device, (dstRegionDesc == nullptr && srcRegionDesc == nullptr) || (dstRegionDesc != nullptr && srcRegionDesc != nullptr), ReturnVoid(),
        "Can't copy texture: 'dstRegionDesc' and 'srcRegionDesc' must be valid pointers or be both NULL.");

    Texture* dstTextureImpl = NRI_GET_IMPL_REF(Texture, &dstTexture);
    Texture* srcTextureImpl = NRI_GET_IMPL_REF(Texture, &srcTexture);

    GetCoreInterface().CmdCopyTexture(GetImpl(), *dstTextureImpl, dstNodeIndex, dstRegionDesc, *srcTextureImpl, srcNodeIndex,
        srcRegionDesc);
}

void CommandBufferVal::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer,
    const TextureDataLayoutDesc& srcDataLayoutDesc)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't upload buffer to texture: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't upload buffer to texture: this operation is allowed only outside render pass.");

    Texture* dstTextureImpl = NRI_GET_IMPL_REF(Texture, &dstTexture);
    Buffer* srcBufferImpl = NRI_GET_IMPL_REF(Buffer, &srcBuffer);

    GetCoreInterface().CmdUploadBufferToTexture(GetImpl(), *dstTextureImpl, dstRegionDesc, *srcBufferImpl, srcDataLayoutDesc);
}

void CommandBufferVal::ReadbackTextureToBuffer(Buffer& dstBuffer, TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture,
    const TextureRegionDesc& srcRegionDesc)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't readback texture to buffer: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't readback texture to buffer: this operation is allowed only outside render pass.");

    Buffer* dstBufferImpl = NRI_GET_IMPL_REF(Buffer, &dstBuffer);
    Texture* srcTextureImpl = NRI_GET_IMPL_REF(Texture, &srcTexture);

    GetCoreInterface().CmdReadbackTextureToBuffer(GetImpl(), *dstBufferImpl, dstDataLayoutDesc, *srcTextureImpl, srcRegionDesc);
}

void CommandBufferVal::Dispatch(uint32_t x, uint32_t y, uint32_t z)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't record dispatch call: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't record dispatch call: this operation is allowed only outside render pass.");

    GetCoreInterface().CmdDispatch(GetImpl(), x, y, z);
}

void CommandBufferVal::DispatchIndirect(const Buffer& buffer, uint64_t offset)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't record dispatch call: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't record dispatch call: this operation is allowed only outside render pass.");

    Buffer* bufferImpl = NRI_GET_IMPL_REF(Buffer, &buffer);

    GetCoreInterface().CmdDispatchIndirect(GetImpl(), *bufferImpl, offset);
}

void CommandBufferVal::PipelineBarrier(const TransitionBarrierDesc* transitionBarriers, const AliasingBarrierDesc* aliasingBarriers, BarrierDependency dependency)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't record pipeline barrier: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't record pipeline barrier: this operation is allowed only outside render pass.");

    TransitionBarrierDesc transitionBarrierImpl;
    if (transitionBarriers)
    {
        transitionBarrierImpl = *transitionBarriers;

        for (uint32_t i = 0; i < transitionBarriers->bufferNum; i++)
        {
            if (!ValidateBufferTransitionBarrierDesc(m_Device, i, transitionBarriers->buffers[i]))
                return;
        }

        for (uint32_t i = 0; i < transitionBarriers->textureNum; i++)
        {
            if (!ValidateTextureTransitionBarrierDesc(m_Device, i, transitionBarriers->textures[i]))
                return;
        }

        transitionBarrierImpl.buffers = STACK_ALLOC(BufferTransitionBarrierDesc, transitionBarriers->bufferNum);
        memcpy((void*)transitionBarrierImpl.buffers, transitionBarriers->buffers, sizeof(BufferTransitionBarrierDesc) * transitionBarriers->bufferNum);
        for (uint32_t i = 0; i < transitionBarrierImpl.bufferNum; i++)
            ((BufferTransitionBarrierDesc*)transitionBarrierImpl.buffers)[i].buffer = NRI_GET_IMPL_PTR(Buffer, transitionBarriers->buffers[i].buffer);

        transitionBarrierImpl.textures = STACK_ALLOC(TextureTransitionBarrierDesc, transitionBarriers->textureNum);
        memcpy((void*)transitionBarrierImpl.textures, transitionBarriers->textures, sizeof(TextureTransitionBarrierDesc) * transitionBarriers->textureNum);
        for (uint32_t i = 0; i < transitionBarrierImpl.textureNum; i++)
            ((TextureTransitionBarrierDesc*)transitionBarrierImpl.textures)[i].texture = NRI_GET_IMPL_PTR(Texture, transitionBarriers->textures[i].texture);

        transitionBarriers = &transitionBarrierImpl;
    }

    AliasingBarrierDesc aliasingBarriersImpl;
    if (aliasingBarriers)
    {
        aliasingBarriersImpl = *aliasingBarriers;

        aliasingBarriersImpl.buffers = STACK_ALLOC(BufferAliasingBarrierDesc, aliasingBarriers->bufferNum);
        memcpy((void*)aliasingBarriersImpl.buffers, aliasingBarriers->buffers, sizeof(BufferAliasingBarrierDesc) * aliasingBarriers->bufferNum);
        for (uint32_t i = 0; i < aliasingBarriersImpl.bufferNum; i++)
        {
            ((BufferAliasingBarrierDesc*)aliasingBarriersImpl.buffers)[i].before = NRI_GET_IMPL_PTR(Buffer, aliasingBarriers->buffers[i].before);
            ((BufferAliasingBarrierDesc*)aliasingBarriersImpl.buffers)[i].after = NRI_GET_IMPL_PTR(Buffer, aliasingBarriers->buffers[i].after);
        }

        aliasingBarriersImpl.textures = STACK_ALLOC(TextureAliasingBarrierDesc, aliasingBarriers->textureNum);
        memcpy((void*)aliasingBarriersImpl.textures, aliasingBarriers->textures, sizeof(TextureAliasingBarrierDesc) * aliasingBarriers->textureNum);
        for (uint32_t i = 0; i < aliasingBarriersImpl.textureNum; i++)
        {
            ((TextureAliasingBarrierDesc*)aliasingBarriersImpl.textures)[i].before = NRI_GET_IMPL_PTR(Texture, aliasingBarriers->textures[i].before);
            ((TextureAliasingBarrierDesc*)aliasingBarriersImpl.textures)[i].after = NRI_GET_IMPL_PTR(Texture, aliasingBarriers->textures[i].after);
        }

        aliasingBarriers = &aliasingBarriersImpl;
    }

    GetCoreInterface().CmdPipelineBarrier(GetImpl(), transitionBarriers, aliasingBarriers, dependency);
}

void CommandBufferVal::BeginQuery(const QueryPool& queryPool, uint32_t offset)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't begin query: the command buffer must be in the recording state.");

    const QueryPoolVal& queryPoolVal = (const QueryPoolVal&)queryPool;

    RETURN_ON_FAILURE(&m_Device, queryPoolVal.GetQueryType() != QueryType::TIMESTAMP, ReturnVoid(),
        "Can't begin query: BeginQuery() is not supported for timestamp queries.");

    if (!queryPoolVal.IsImported())
    {
        RETURN_ON_FAILURE(&m_Device, offset < queryPoolVal.GetQueryNum(), ReturnVoid(),
        "Can't begin query: the offset ('%u') is out of range.", offset);

        ValidationCommandUseQuery& validationCommand = AllocateValidationCommand<ValidationCommandUseQuery>();
        validationCommand.type = ValidationCommandType::BEGIN_QUERY;
        validationCommand.queryPool = const_cast<QueryPool*>(&queryPool);
        validationCommand.queryPoolOffset = offset;
    }

    QueryPool* queryPoolImpl = NRI_GET_IMPL_REF(QueryPool, &queryPool);

    GetCoreInterface().CmdBeginQuery(GetImpl(), *queryPoolImpl, offset);
}

void CommandBufferVal::EndQuery(const QueryPool& queryPool, uint32_t offset)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't end query: the command buffer must be in the recording state.");

    const QueryPoolVal& queryPoolVal = (const QueryPoolVal&)queryPool;

    if (!queryPoolVal.IsImported())
    {
        RETURN_ON_FAILURE(&m_Device, offset < queryPoolVal.GetQueryNum(), ReturnVoid(),
            "Can't end query: the offset ('%u') is out of range.", offset);

        ValidationCommandUseQuery& validationCommand = AllocateValidationCommand<ValidationCommandUseQuery>();
        validationCommand.type = ValidationCommandType::END_QUERY;
        validationCommand.queryPool = const_cast<QueryPool*>(&queryPool);
        validationCommand.queryPoolOffset = offset;
    }

    QueryPool* queryPoolImpl = NRI_GET_IMPL_REF(QueryPool, &queryPool);

    GetCoreInterface().CmdEndQuery(GetImpl(), *queryPoolImpl, offset);
}

void CommandBufferVal::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't copy queries: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't copy queries: this operation is allowed only outside render pass.");

    const QueryPoolVal& queryPoolVal = (const QueryPoolVal&)queryPool;

    if (!queryPoolVal.IsImported())
    {
        RETURN_ON_FAILURE(&m_Device, offset + num <= queryPoolVal.GetQueryNum(), ReturnVoid(),
            "Can't copy queries: offset + num ('%u') is out of range.", offset + num);
    }

    QueryPool* queryPoolImpl = NRI_GET_IMPL_REF(QueryPool, &queryPool);
    Buffer* dstBufferImpl = NRI_GET_IMPL_REF(Buffer, &dstBuffer);

    GetCoreInterface().CmdCopyQueries(GetImpl(), *queryPoolImpl, offset, num, *dstBufferImpl, dstOffset);
}

void CommandBufferVal::ResetQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't reset queries: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't reset queries: this operation is allowed only outside render pass.");

    const QueryPoolVal& queryPoolVal = (const QueryPoolVal&)queryPool;

    if (!queryPoolVal.IsImported())
    {
        RETURN_ON_FAILURE(&m_Device, offset + num <= queryPoolVal.GetQueryNum(), ReturnVoid(),
            "Can't reset queries: offset + num ('%u') is out of range.", offset + num);

        ValidationCommandResetQuery& validationCommand = AllocateValidationCommand<ValidationCommandResetQuery>();
        validationCommand.type = ValidationCommandType::RESET_QUERY;
        validationCommand.queryPool = const_cast<QueryPool*>(&queryPool);
        validationCommand.queryPoolOffset = offset;
        validationCommand.queryNum = num;
    }

    QueryPool* queryPoolImpl = NRI_GET_IMPL_REF(QueryPool, &queryPool);

    GetCoreInterface().CmdResetQueries(GetImpl(), *queryPoolImpl, offset, num);
}

void CommandBufferVal::BeginAnnotation(const char* name)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't copy queries: the command buffer must be in the recording state.");

    m_AnnotationStack++;
    GetCoreInterface().CmdBeginAnnotation(GetImpl(), name);
}

void CommandBufferVal::EndAnnotation()
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't copy queries: the command buffer must be in the recording state.");

    GetCoreInterface().CmdEndAnnotation(GetImpl());
    m_AnnotationStack--;
}

void CommandBufferVal::Destroy()
{
    GetCoreInterface().DestroyCommandBuffer(GetImpl());
    Deallocate(m_Device.GetStdAllocator(), this);
}

void CommandBufferVal::BuildTopLevelAccelerationStructure(uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset,
    AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't build TLAS: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't build TLAS: this operation is allowed only outside render pass.");

    BufferVal& bufferVal = (BufferVal&)buffer;
    BufferVal& scratchVal = (BufferVal&)scratch;

    RETURN_ON_FAILURE(&m_Device, bufferOffset < bufferVal.GetDesc().size, ReturnVoid(),
        "Can't update TLAS: 'bufferOffset' is out of bounds.");

    RETURN_ON_FAILURE(&m_Device, scratchOffset < scratchVal.GetDesc().size, ReturnVoid(),
        "Can't update TLAS: 'scratchOffset' is out of bounds.");

    AccelerationStructure& dstImpl = *NRI_GET_IMPL_REF(AccelerationStructure, &dst);
    Buffer& scratchImpl = *NRI_GET_IMPL_REF(Buffer, &scratch);
    Buffer& bufferImpl = *NRI_GET_IMPL_REF(Buffer, &buffer);

    m_RayTracingAPI.CmdBuildTopLevelAccelerationStructure(GetImpl(), instanceNum, bufferImpl, bufferOffset, flags, dstImpl, scratchImpl, scratchOffset);
}

void CommandBufferVal::BuildBottomLevelAccelerationStructure(uint32_t geometryObjectNum, const GeometryObject* geometryObjects,
    AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't build BLAS: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't build BLAS: this operation is allowed only outside render pass.");

    RETURN_ON_FAILURE(&m_Device, geometryObjects != nullptr, ReturnVoid(),
        "Can't update BLAS: 'geometryObjects' is invalid.");

    BufferVal& scratchVal = (BufferVal&)scratch;

    RETURN_ON_FAILURE(&m_Device, scratchOffset < scratchVal.GetDesc().size, ReturnVoid(),
        "Can't build BLAS: 'scratchOffset' is out of bounds.");

    AccelerationStructure& dstImpl = *NRI_GET_IMPL_REF(AccelerationStructure, &dst);
    Buffer& scratchImpl = *NRI_GET_IMPL_REF(Buffer, &scratch);

    Vector<GeometryObject> objectImplArray(geometryObjectNum, m_Device.GetStdAllocator());
    ConvertGeometryObjectsVal(objectImplArray.data(), geometryObjects, geometryObjectNum);

    m_RayTracingAPI.CmdBuildBottomLevelAccelerationStructure(GetImpl(), geometryObjectNum, objectImplArray.data(), flags, dstImpl, scratchImpl, scratchOffset);
}

void CommandBufferVal::UpdateTopLevelAccelerationStructure(uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset,
    AccelerationStructureBuildBits flags, AccelerationStructure& dst, AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't update TLAS: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't update TLAS: this operation is allowed only outside render pass.");

    BufferVal& bufferVal = (BufferVal&)buffer;
    BufferVal& scratchVal = (BufferVal&)scratch;

    RETURN_ON_FAILURE(&m_Device, bufferOffset < bufferVal.GetDesc().size, ReturnVoid(),
        "Can't update TLAS: 'bufferOffset' is out of bounds.");

    RETURN_ON_FAILURE(&m_Device, scratchOffset < scratchVal.GetDesc().size, ReturnVoid(),
        "Can't update TLAS: 'scratchOffset' is out of bounds.");

    AccelerationStructure& dstImpl = *NRI_GET_IMPL_REF(AccelerationStructure, &dst);
    AccelerationStructure& srcImpl = *NRI_GET_IMPL_REF(AccelerationStructure, &src);
    Buffer& scratchImpl = *NRI_GET_IMPL_REF(Buffer, &scratch);
    Buffer& bufferImpl = *NRI_GET_IMPL_REF(Buffer, &buffer);

    m_RayTracingAPI.CmdUpdateTopLevelAccelerationStructure(GetImpl(), instanceNum, bufferImpl, bufferOffset, flags, dstImpl, srcImpl, scratchImpl, scratchOffset);
}

void CommandBufferVal::UpdateBottomLevelAccelerationStructure(uint32_t geometryObjectNum, const GeometryObject* geometryObjects,
    AccelerationStructureBuildBits flags, AccelerationStructure& dst, AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't update BLAS: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't update BLAS: this operation is allowed only outside render pass.");

    RETURN_ON_FAILURE(&m_Device, geometryObjects != nullptr, ReturnVoid(),
        "Can't update BLAS: 'geometryObjects' is invalid.");

    BufferVal& scratchVal = (BufferVal&)scratch;

    RETURN_ON_FAILURE(&m_Device, scratchOffset < scratchVal.GetDesc().size, ReturnVoid(),
        "Can't update BLAS: 'scratchOffset' is out of bounds.");

    AccelerationStructure& dstImpl = *NRI_GET_IMPL_REF(AccelerationStructure, &dst);
    AccelerationStructure& srcImpl = *NRI_GET_IMPL_REF(AccelerationStructure, &src);
    Buffer& scratchImpl = *NRI_GET_IMPL_REF(Buffer, &scratch);

    Vector<GeometryObject> objectImplArray(geometryObjectNum, m_Device.GetStdAllocator());
    ConvertGeometryObjectsVal(objectImplArray.data(), geometryObjects, geometryObjectNum);

    m_RayTracingAPI.CmdUpdateBottomLevelAccelerationStructure(GetImpl(), geometryObjectNum, objectImplArray.data(), flags, dstImpl, srcImpl, scratchImpl, scratchOffset);
}

void CommandBufferVal::CopyAccelerationStructure(AccelerationStructure& dst, AccelerationStructure& src, CopyMode copyMode)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't copy AS: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't copy AS: this operation is allowed only outside render pass.");

    RETURN_ON_FAILURE(&m_Device, copyMode < CopyMode::MAX_NUM, ReturnVoid(),
        "Can't copy AS: 'copyMode' is invalid.");

    AccelerationStructure& dstImpl = *NRI_GET_IMPL_REF(AccelerationStructure, &dst);
    AccelerationStructure& srcImpl = *NRI_GET_IMPL_REF(AccelerationStructure, &src);

    m_RayTracingAPI.CmdCopyAccelerationStructure(GetImpl(), dstImpl, srcImpl, copyMode);
}

void CommandBufferVal::WriteAccelerationStructureSize(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryOffset)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't write AS size: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't write AS size: this operation is allowed only outside render pass.");

    RETURN_ON_FAILURE(&m_Device, accelerationStructures != nullptr, ReturnVoid(),
        "Can't write AS size: 'accelerationStructures' is invalid.");

    AccelerationStructure** accelerationStructureArray = STACK_ALLOC(AccelerationStructure*, accelerationStructureNum);
    for (uint32_t i = 0; i < accelerationStructureNum; i++)
    {
        RETURN_ON_FAILURE(&m_Device, accelerationStructures[i] != nullptr, ReturnVoid(),
            "Can't write AS size: 'accelerationStructures[%u]' is invalid.", i);

        accelerationStructureArray[i] = NRI_GET_IMPL_PTR(AccelerationStructure, accelerationStructures[i]);
    }

    QueryPool& queryPoolImpl = *NRI_GET_IMPL_REF(QueryPool, &queryPool);

    m_RayTracingAPI.CmdWriteAccelerationStructureSize(GetImpl(), accelerationStructures, accelerationStructureNum, queryPoolImpl, queryOffset);
}

void CommandBufferVal::DispatchRays(const DispatchRaysDesc& dispatchRaysDesc)
{
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(),
        "Can't record ray tracing dispatch: the command buffer must be in the recording state.");

    RETURN_ON_FAILURE(&m_Device, m_FrameBuffer == nullptr, ReturnVoid(),
        "Can't record ray tracing dispatch: this operation is allowed only outside render pass.");

    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.raygenShader.buffer != nullptr, ReturnVoid(),
        "Can't record ray tracing dispatch: 'dispatchRaysDesc.raygenShader.buffer' is invalid.");

    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.raygenShader.size != 0, ReturnVoid(),
        "Can't record ray tracing dispatch: 'dispatchRaysDesc.raygenShader.size' is 0.");

    const uint64_t SBTAlignment = m_Device.GetDesc().rayTracingShaderTableAligment;

    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.raygenShader.offset % SBTAlignment == 0, ReturnVoid(),
        "Can't record ray tracing dispatch: 'dispatchRaysDesc.raygenShader.offset' is misaligned.");

    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.missShaders.offset % SBTAlignment == 0, ReturnVoid(),
        "Can't record ray tracing dispatch: 'dispatchRaysDesc.missShaders.offset' is misaligned.");

    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.hitShaderGroups.offset % SBTAlignment == 0, ReturnVoid(),
        "Can't record ray tracing dispatch: 'dispatchRaysDesc.hitShaderGroups.offset' is misaligned.");

    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.callableShaders.offset % SBTAlignment == 0, ReturnVoid(),
        "Can't record ray tracing dispatch: 'dispatchRaysDesc.callableShaders.offset' is misaligned.");

    auto dispatchRaysDescImpl = dispatchRaysDesc;
    dispatchRaysDescImpl.raygenShader.buffer = NRI_GET_IMPL_PTR(Buffer, dispatchRaysDesc.raygenShader.buffer);
    dispatchRaysDescImpl.missShaders.buffer = NRI_GET_IMPL_PTR(Buffer, dispatchRaysDesc.missShaders.buffer);
    dispatchRaysDescImpl.hitShaderGroups.buffer = NRI_GET_IMPL_PTR(Buffer, dispatchRaysDesc.hitShaderGroups.buffer);
    dispatchRaysDescImpl.callableShaders.buffer = NRI_GET_IMPL_PTR(Buffer, dispatchRaysDesc.callableShaders.buffer);

    m_RayTracingAPI.CmdDispatchRays(GetImpl(), dispatchRaysDescImpl);
}

void CommandBufferVal::DispatchMeshTasks(uint32_t x, uint32_t y, uint32_t z)
{
    m_MeshShaderAPI.CmdDispatchMeshTasks(GetImpl(), x, y, z);
}

template<typename Command>
Command& CommandBufferVal::AllocateValidationCommand()
{
    const size_t commandSize = sizeof(Command);
    const size_t newSize = m_ValidationCommands.size() + commandSize;
    const size_t capacity = m_ValidationCommands.capacity();

    if (newSize > capacity)
        m_ValidationCommands.reserve(std::max(capacity + (capacity >> 1), newSize));

    const size_t offset = m_ValidationCommands.size();
    m_ValidationCommands.resize(newSize);

    return *(Command*)(m_ValidationCommands.data() + offset);
}

static bool ValidateBufferTransitionBarrierDesc(const DeviceVal& device, uint32_t i, const BufferTransitionBarrierDesc& bufferTransitionBarrierDesc)
{
    RETURN_ON_FAILURE(&device, bufferTransitionBarrierDesc.buffer != nullptr, false,
        "Can't record pipeline barrier: 'transitionBarriers->buffers[%u].buffer' is invalid.", i);

    const BufferVal& bufferVal = *(const BufferVal*)bufferTransitionBarrierDesc.buffer;

    RETURN_ON_FAILURE(&device, IsAccessMaskSupported(bufferVal.GetDesc().usageMask, bufferTransitionBarrierDesc.prevAccess), false,
        "Can't record pipeline barrier: 'transitionBarriers->buffers[%u].prevAccess' is not supported by the usage mask of the buffer ('%s').",
        i, bufferVal.GetDebugName());

    RETURN_ON_FAILURE(&device, IsAccessMaskSupported(bufferVal.GetDesc().usageMask, bufferTransitionBarrierDesc.nextAccess), false,
        "Can't record pipeline barrier: 'transitionBarriers->buffers[%u].nextAccess' is not supported by the usage mask of the buffer ('%s').",
        i, bufferVal.GetDebugName());

    return true;
}

static bool ValidateTextureTransitionBarrierDesc(const DeviceVal& device, uint32_t i, const TextureTransitionBarrierDesc& textureTransitionBarrierDesc)
{
    RETURN_ON_FAILURE(&device, textureTransitionBarrierDesc.texture != nullptr, false,
        "Can't record pipeline barrier: 'transitionBarriers->textures[%u].texture' is invalid.", i);

    const TextureVal& textureVal = *(const TextureVal*)textureTransitionBarrierDesc.texture;

    RETURN_ON_FAILURE(&device, IsAccessMaskSupported(textureVal.GetDesc().usageMask, textureTransitionBarrierDesc.prevAccess), false,
        "Can't record pipeline barrier: 'transitionBarriers->textures[%u].prevAccess' is not supported by the usage mask of the texture ('%s').",
        i, textureVal.GetDebugName());

    RETURN_ON_FAILURE(&device, IsAccessMaskSupported(textureVal.GetDesc().usageMask, textureTransitionBarrierDesc.nextAccess), false,
        "Can't record pipeline barrier: 'transitionBarriers->textures[%u].nextAccess' is not supported by the usage mask of the texture ('%s').",
        i, textureVal.GetDebugName());

    RETURN_ON_FAILURE(&device, IsTextureLayoutSupported(textureVal.GetDesc().usageMask, textureTransitionBarrierDesc.prevLayout), false,
        "Can't record pipeline barrier: 'transitionBarriers->textures[%u].prevLayout' is not supported by the usage mask of the texture ('%s').",
        i, textureVal.GetDebugName());

    RETURN_ON_FAILURE(&device, IsTextureLayoutSupported(textureVal.GetDesc().usageMask, textureTransitionBarrierDesc.nextLayout), false,
        "Can't record pipeline barrier: 'transitionBarriers->textures[%u].nextLayout' is not supported by the usage mask of the texture ('%s').",
        i, textureVal.GetDebugName());

    return true;
}


#include "CommandBufferVal.hpp"
