/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include <inttypes.h>

#include "SharedExternal.h"
#include "DeviceBase.h"
#include "DeviceVal.h"
#include "SharedVal.h"
#include "CommandBufferVal.h"

#include "BufferVal.h"
#include "DescriptorVal.h"
#include "DescriptorSetVal.h"
#include "DescriptorPoolVal.h"
#include "PipelineLayoutVal.h"
#include "PipelineVal.h"
#include "QueryPoolVal.h"
#include "TextureVal.h"
#include "AccelerationStructureVal.h"

using namespace nri;

void ConvertGeometryObjectsVal(GeometryObject* destObjects, const GeometryObject* sourceObjects, uint32_t objectNum);

static bool ValidateBufferTransitionBarrierDesc(const DeviceVal& device, uint32_t i, const BufferTransitionBarrierDesc& bufferTransitionBarrierDesc) {
    const BufferVal& bufferVal = *(const BufferVal*)bufferTransitionBarrierDesc.buffer;

    RETURN_ON_FAILURE(&device, bufferTransitionBarrierDesc.buffer != nullptr, false, "CmdPipelineBarrier: 'transitionBarriers->buffers[%u].buffer' is NULL", i);
    RETURN_ON_FAILURE(
        &device, IsAccessMaskSupported(bufferVal.GetDesc().usageMask, bufferTransitionBarrierDesc.prevAccess), false,
        "CmdPipelineBarrier: 'transitionBarriers->buffers[%u].prevAccess' is not supported by the usage mask of the buffer ('%s')", i, bufferVal.GetDebugName()
    );
    RETURN_ON_FAILURE(
        &device, IsAccessMaskSupported(bufferVal.GetDesc().usageMask, bufferTransitionBarrierDesc.nextAccess), false,
        "CmdPipelineBarrier: 'transitionBarriers->buffers[%u].nextAccess' is not supported by the usage mask of the buffer ('%s')", i, bufferVal.GetDebugName()
    );

    return true;
}

static bool ValidateTextureTransitionBarrierDesc(const DeviceVal& device, uint32_t i, const TextureTransitionBarrierDesc& textureTransitionBarrierDesc) {
    const TextureVal& textureVal = *(const TextureVal*)textureTransitionBarrierDesc.texture;

    RETURN_ON_FAILURE(&device, textureTransitionBarrierDesc.texture != nullptr, false, "CmdPipelineBarrier: 'transitionBarriers->textures[%u].texture' is NULL", i);
    RETURN_ON_FAILURE(
        &device, IsAccessMaskSupported(textureVal.GetDesc().usageMask, textureTransitionBarrierDesc.prevState.acessBits), false,
        "CmdPipelineBarrier: 'transitionBarriers->textures[%u].prevAccess' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName()
    );
    RETURN_ON_FAILURE(
        &device, IsAccessMaskSupported(textureVal.GetDesc().usageMask, textureTransitionBarrierDesc.nextState.acessBits), false,
        "CmdPipelineBarrier: 'transitionBarriers->textures[%u].nextAccess' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName()
    );
    RETURN_ON_FAILURE(
        &device, IsTextureLayoutSupported(textureVal.GetDesc().usageMask, textureTransitionBarrierDesc.prevState.layout), false,
        "CmdPipelineBarrier: 'transitionBarriers->textures[%u].prevLayout' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName()
    );
    RETURN_ON_FAILURE(
        &device, IsTextureLayoutSupported(textureVal.GetDesc().usageMask, textureTransitionBarrierDesc.nextState.layout), false,
        "CmdPipelineBarrier: 'transitionBarriers->textures[%u].nextLayout' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName()
    );

    return true;
}

void CommandBufferVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetCommandBufferDebugName(*GetImpl(), name);
}

Result CommandBufferVal::Begin(const DescriptorPool* descriptorPool, uint32_t nodeIndex) {
    RETURN_ON_FAILURE(&m_Device, !m_IsRecordingStarted, Result::FAILURE, "BeginCommandBuffer: already in the recording state");

    DescriptorPool* descriptorPoolImpl = NRI_GET_IMPL(DescriptorPool, descriptorPool);

    Result result = GetCoreInterface().BeginCommandBuffer(*GetImpl(), descriptorPoolImpl, nodeIndex);
    if (result == Result::SUCCESS)
        m_IsRecordingStarted = true;

    m_ValidationCommands.clear();

    return result;
}

Result CommandBufferVal::End() {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, Result::FAILURE, "EndCommandBuffer: not in the recording state");

    if (m_AnnotationStack > 0)
        REPORT_ERROR(&m_Device, "EndCommandBuffer: 'CmdBeginAnnotation' is called more times than 'CmdEndAnnotation'");
    else if (m_AnnotationStack < 0)
        REPORT_ERROR(&m_Device, "EndCommandBuffer: 'CmdEndAnnotation' is called more times than 'CmdBeginAnnotation'");

    Result result = GetCoreInterface().EndCommandBuffer(*GetImpl());

    if (result == Result::SUCCESS)
        m_IsRecordingStarted = m_IsWrapped;

    return result;
}

void CommandBufferVal::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdSetViewports: the command buffer must be in the recording state");

    if (viewportNum == 0)
        return;

    RETURN_ON_FAILURE(&m_Device, viewports != nullptr, ReturnVoid(), "CmdSetViewports: 'viewports' is NULL");

    GetCoreInterface().CmdSetViewports(*GetImpl(), viewports, viewportNum);
}

void CommandBufferVal::SetScissors(const Rect* rects, uint32_t rectNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdSetScissors: the command buffer must be in the recording state");

    if (rectNum == 0)
        return;

    RETURN_ON_FAILURE(&m_Device, rects != nullptr, ReturnVoid(), "CmdSetScissors: 'rects' is NULL");

    GetCoreInterface().CmdSetScissors(*GetImpl(), rects, rectNum);
}

void CommandBufferVal::SetDepthBounds(float boundsMin, float boundsMax) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdSetDepthBounds: the command buffer must be in the recording state");

    GetCoreInterface().CmdSetDepthBounds(*GetImpl(), boundsMin, boundsMax);
}

void CommandBufferVal::SetStencilReference(uint8_t reference) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdSetStencilReference: the command buffer must be in the recording state");

    GetCoreInterface().CmdSetStencilReference(*GetImpl(), reference);
}

void CommandBufferVal::SetSamplePositions(const SamplePosition* positions, uint32_t positionNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdSetSamplePositions: the command buffer must be in the recording state");

    GetCoreInterface().CmdSetSamplePositions(*GetImpl(), positions, positionNum);
}

void CommandBufferVal::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdClearAttachments: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "CmdClearAttachments: must be called inside 'CmdBeginRendering/CmdEndRendering'");

    GetCoreInterface().CmdClearAttachments(*GetImpl(), clearDescs, clearDescNum, rects, rectNum);
}

void CommandBufferVal::ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdClearStorageBuffer: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdClearStorageBuffer: must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, clearDesc.storageBuffer != nullptr, ReturnVoid(), "CmdClearStorageBuffer: 'clearDesc.storageBuffer' is NULL");

    auto clearDescImpl = clearDesc;
    clearDescImpl.storageBuffer = NRI_GET_IMPL(Descriptor, clearDesc.storageBuffer);

    GetCoreInterface().CmdClearStorageBuffer(*GetImpl(), clearDescImpl);
}

void CommandBufferVal::ClearStorageTexture(const ClearStorageTextureDesc& clearDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdClearStorageTexture: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdClearStorageTexture: must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, clearDesc.storageTexture != nullptr, ReturnVoid(), "CmdClearStorageTexture: 'clearDesc.storageTexture' is NULL");

    auto clearDescImpl = clearDesc;
    clearDescImpl.storageTexture = NRI_GET_IMPL(Descriptor, clearDesc.storageTexture);

    GetCoreInterface().CmdClearStorageTexture(*GetImpl(), clearDescImpl);
}

void CommandBufferVal::BeginRendering(const AttachmentsDesc& attachmentsDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdBeginRendering: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdBeginRendering: 'CmdBeginRendering' has been already called");

    Descriptor** colors = STACK_ALLOC(Descriptor*, attachmentsDesc.colorNum);
    for (uint32_t i = 0; i < attachmentsDesc.colorNum; i++)
        colors[i] = NRI_GET_IMPL(Descriptor, attachmentsDesc.colors[i]);

    AttachmentsDesc attachmentsDescImpl = {};
    attachmentsDescImpl.depthStencil = NRI_GET_IMPL(Descriptor, attachmentsDesc.depthStencil);
    attachmentsDescImpl.colors = colors;
    attachmentsDescImpl.colorNum = attachmentsDesc.colorNum;

    GetCoreInterface().CmdBeginRendering(*GetImpl(), attachmentsDescImpl);

    m_IsRenderPass = true;
}

void CommandBufferVal::EndRendering() {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdEndRendering: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "CmdEndRendering: 'CmdBeginRendering' has not been called");

    GetCoreInterface().CmdEndRendering(*GetImpl());

    m_IsRenderPass = false;
}

void CommandBufferVal::SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdSetVertexBuffers: the command buffer must be in the recording state");

    Buffer** buffersImpl = STACK_ALLOC(Buffer*, bufferNum);
    for (uint32_t i = 0; i < bufferNum; i++)
        buffersImpl[i] = NRI_GET_IMPL(Buffer, buffers[i]);

    GetCoreInterface().CmdSetVertexBuffers(*GetImpl(), baseSlot, bufferNum, buffersImpl, offsets);
}

void CommandBufferVal::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdSetIndexBuffer: the command buffer must be in the recording state");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);

    GetCoreInterface().CmdSetIndexBuffer(*GetImpl(), *bufferImpl, offset, indexType);
}

void CommandBufferVal::SetPipelineLayout(const PipelineLayout& pipelineLayout) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdSetPipelineLayout: the command buffer must be in the recording state");

    PipelineLayout* pipelineLayoutImpl = NRI_GET_IMPL(PipelineLayout, &pipelineLayout);

    GetCoreInterface().CmdSetPipelineLayout(*GetImpl(), *pipelineLayoutImpl);
}

void CommandBufferVal::SetPipeline(const Pipeline& pipeline) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdSetPipeline: the command buffer must be in the recording state");

    Pipeline* pipelineImpl = NRI_GET_IMPL(Pipeline, &pipeline);

    GetCoreInterface().CmdSetPipeline(*GetImpl(), *pipelineImpl);
}

void CommandBufferVal::SetDescriptorPool(const DescriptorPool& descriptorPool) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdSetDescriptorPool: the command buffer must be in the recording state");

    DescriptorPool* descriptorPoolImpl = NRI_GET_IMPL(DescriptorPool, &descriptorPool);

    GetCoreInterface().CmdSetDescriptorPool(*GetImpl(), *descriptorPoolImpl);
}

void CommandBufferVal::SetDescriptorSet(uint32_t setIndexInPipelineLayout, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdSetDescriptorSet: the command buffer must be in the recording state");

    DescriptorSet* descriptorSetImpl = NRI_GET_IMPL(DescriptorSet, &descriptorSet);

    GetCoreInterface().CmdSetDescriptorSet(*GetImpl(), setIndexInPipelineLayout, *descriptorSetImpl, dynamicConstantBufferOffsets);
}

void CommandBufferVal::SetConstants(uint32_t pushConstantIndex, const void* data, uint32_t size) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdSetConstants: the command buffer must be in the recording state");

    GetCoreInterface().CmdSetConstants(*GetImpl(), pushConstantIndex, data, size);
}

void CommandBufferVal::Draw(uint32_t vertexNum, uint32_t instanceNum, uint32_t baseVertex, uint32_t baseInstance) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdDraw: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "CmdDraw: must be called inside 'CmdBeginRendering/CmdEndRendering'");

    GetCoreInterface().CmdDraw(*GetImpl(), vertexNum, instanceNum, baseVertex, baseInstance);
}

void CommandBufferVal::DrawIndexed(uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdDrawIndexed: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "CmdDrawIndexed: must be called inside 'CmdBeginRendering/CmdEndRendering'");

    GetCoreInterface().CmdDrawIndexed(*GetImpl(), indexNum, instanceNum, baseIndex, baseVertex, baseInstance);
}

void CommandBufferVal::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdDrawIndirect: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "CmdDrawIndirect: must be called inside 'CmdBeginRendering/CmdEndRendering'");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);

    GetCoreInterface().CmdDrawIndirect(*GetImpl(), *bufferImpl, offset, drawNum, stride);
}

void CommandBufferVal::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdDrawIndexedIndirect: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "CmdDrawIndexedIndirect: must be called inside 'CmdBeginRendering/CmdEndRendering'");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);

    GetCoreInterface().CmdDrawIndexedIndirect(*GetImpl(), *bufferImpl, offset, drawNum, stride);
}

void CommandBufferVal::CopyBuffer(Buffer& dstBuffer, uint32_t dstNodeIndex, uint64_t dstOffset, const Buffer& srcBuffer, uint32_t srcNodeIndex, uint64_t srcOffset, uint64_t size) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdCopyBuffer: the command buffer must be in the recording state");

    if (size == WHOLE_SIZE) {
        const BufferDesc& dstDesc = ((BufferVal&)dstBuffer).GetDesc();
        const BufferDesc& srcDesc = ((BufferVal&)srcBuffer).GetDesc();

        if (dstDesc.size != srcDesc.size)
            REPORT_WARNING(&m_Device, "WHOLE_SIZE is used but 'dstBuffer' and 'srcBuffer' have diffenet sizes");
    }

    Buffer* dstBufferImpl = NRI_GET_IMPL(Buffer, &dstBuffer);
    Buffer* srcBufferImpl = NRI_GET_IMPL(Buffer, &srcBuffer);

    GetCoreInterface().CmdCopyBuffer(*GetImpl(), *dstBufferImpl, dstNodeIndex, dstOffset, *srcBufferImpl, srcNodeIndex, srcOffset, size);
}

void CommandBufferVal::CopyTexture(
    Texture& dstTexture, uint32_t dstNodeIndex, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, uint32_t srcNodeIndex, const TextureRegionDesc* srcRegionDesc
) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdCopyTexture: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdCopyTexture: must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(
        &m_Device, (dstRegionDesc == nullptr && srcRegionDesc == nullptr) || (dstRegionDesc != nullptr && srcRegionDesc != nullptr), ReturnVoid(),
        "CmdCopyTexture: 'dstRegionDesc' and 'srcRegionDesc' must be valid pointers or both NULL"
    );

    Texture* dstTextureImpl = NRI_GET_IMPL(Texture, &dstTexture);
    Texture* srcTextureImpl = NRI_GET_IMPL(Texture, &srcTexture);

    GetCoreInterface().CmdCopyTexture(*GetImpl(), *dstTextureImpl, dstNodeIndex, dstRegionDesc, *srcTextureImpl, srcNodeIndex, srcRegionDesc);
}

void CommandBufferVal::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdUploadBufferToTexture: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdUploadBufferToTexture: must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Texture* dstTextureImpl = NRI_GET_IMPL(Texture, &dstTexture);
    Buffer* srcBufferImpl = NRI_GET_IMPL(Buffer, &srcBuffer);

    GetCoreInterface().CmdUploadBufferToTexture(*GetImpl(), *dstTextureImpl, dstRegionDesc, *srcBufferImpl, srcDataLayoutDesc);
}

void CommandBufferVal::ReadbackTextureToBuffer(Buffer& dstBuffer, TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdReadbackTextureToBuffer: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdReadbackTextureToBuffer: must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Buffer* dstBufferImpl = NRI_GET_IMPL(Buffer, &dstBuffer);
    Texture* srcTextureImpl = NRI_GET_IMPL(Texture, &srcTexture);

    GetCoreInterface().CmdReadbackTextureToBuffer(*GetImpl(), *dstBufferImpl, dstDataLayoutDesc, *srcTextureImpl, srcRegionDesc);
}

void CommandBufferVal::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdDispatch: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdDispatch: must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    GetCoreInterface().CmdDispatch(*GetImpl(), x, y, z);
}

void CommandBufferVal::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdDispatchIndirect: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdDispatchIndirect: must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);

    GetCoreInterface().CmdDispatchIndirect(*GetImpl(), *bufferImpl, offset);
}

void CommandBufferVal::PipelineBarrier(const TransitionBarrierDesc* transitionBarriers, const AliasingBarrierDesc* aliasingBarriers, BarrierDependency dependency) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdPipelineBarrier: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdPipelineBarrier: must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    TransitionBarrierDesc transitionBarrierImpl;
    if (transitionBarriers) {
        transitionBarrierImpl = *transitionBarriers;

        for (uint32_t i = 0; i < transitionBarriers->bufferNum; i++) {
            if (!ValidateBufferTransitionBarrierDesc(m_Device, i, transitionBarriers->buffers[i]))
                return;
        }

        for (uint32_t i = 0; i < transitionBarriers->textureNum; i++) {
            if (!ValidateTextureTransitionBarrierDesc(m_Device, i, transitionBarriers->textures[i]))
                return;
        }

        transitionBarrierImpl.buffers = STACK_ALLOC(BufferTransitionBarrierDesc, transitionBarriers->bufferNum);
        memcpy((void*)transitionBarrierImpl.buffers, transitionBarriers->buffers, sizeof(BufferTransitionBarrierDesc) * transitionBarriers->bufferNum);
        for (uint32_t i = 0; i < transitionBarrierImpl.bufferNum; i++)
            ((BufferTransitionBarrierDesc*)transitionBarrierImpl.buffers)[i].buffer = NRI_GET_IMPL(Buffer, transitionBarriers->buffers[i].buffer);

        transitionBarrierImpl.textures = STACK_ALLOC(TextureTransitionBarrierDesc, transitionBarriers->textureNum);
        memcpy((void*)transitionBarrierImpl.textures, transitionBarriers->textures, sizeof(TextureTransitionBarrierDesc) * transitionBarriers->textureNum);
        for (uint32_t i = 0; i < transitionBarrierImpl.textureNum; i++)
            ((TextureTransitionBarrierDesc*)transitionBarrierImpl.textures)[i].texture = NRI_GET_IMPL(Texture, transitionBarriers->textures[i].texture);

        transitionBarriers = &transitionBarrierImpl;
    }

    AliasingBarrierDesc aliasingBarriersImpl;
    if (aliasingBarriers) {
        aliasingBarriersImpl = *aliasingBarriers;

        aliasingBarriersImpl.buffers = STACK_ALLOC(BufferAliasingBarrierDesc, aliasingBarriers->bufferNum);
        memcpy((void*)aliasingBarriersImpl.buffers, aliasingBarriers->buffers, sizeof(BufferAliasingBarrierDesc) * aliasingBarriers->bufferNum);
        for (uint32_t i = 0; i < aliasingBarriersImpl.bufferNum; i++) {
            ((BufferAliasingBarrierDesc*)aliasingBarriersImpl.buffers)[i].before = NRI_GET_IMPL(Buffer, aliasingBarriers->buffers[i].before);
            ((BufferAliasingBarrierDesc*)aliasingBarriersImpl.buffers)[i].after = NRI_GET_IMPL(Buffer, aliasingBarriers->buffers[i].after);
        }

        aliasingBarriersImpl.textures = STACK_ALLOC(TextureAliasingBarrierDesc, aliasingBarriers->textureNum);
        memcpy((void*)aliasingBarriersImpl.textures, aliasingBarriers->textures, sizeof(TextureAliasingBarrierDesc) * aliasingBarriers->textureNum);
        for (uint32_t i = 0; i < aliasingBarriersImpl.textureNum; i++) {
            ((TextureAliasingBarrierDesc*)aliasingBarriersImpl.textures)[i].before = NRI_GET_IMPL(Texture, aliasingBarriers->textures[i].before);
            ((TextureAliasingBarrierDesc*)aliasingBarriersImpl.textures)[i].after = NRI_GET_IMPL(Texture, aliasingBarriers->textures[i].after);
        }

        aliasingBarriers = &aliasingBarriersImpl;
    }

    GetCoreInterface().CmdPipelineBarrier(*GetImpl(), transitionBarriers, aliasingBarriers, dependency);
}

void CommandBufferVal::BeginQuery(const QueryPool& queryPool, uint32_t offset) {
    const QueryPoolVal& queryPoolVal = (const QueryPoolVal&)queryPool;

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdBeginQuery: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, queryPoolVal.GetQueryType() != QueryType::TIMESTAMP, ReturnVoid(), "CmdBeginQuery: 'BeginQuery' is not supported for timestamp queries");

    if (!queryPoolVal.IsImported()) {
        RETURN_ON_FAILURE(&m_Device, offset < queryPoolVal.GetQueryNum(), ReturnVoid(), "CmdBeginQuery: 'offset = %u' is out of range", offset);

        ValidationCommandUseQuery& validationCommand = AllocateValidationCommand<ValidationCommandUseQuery>();
        validationCommand.type = ValidationCommandType::BEGIN_QUERY;
        validationCommand.queryPool = const_cast<QueryPool*>(&queryPool);
        validationCommand.queryPoolOffset = offset;
    }

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);

    GetCoreInterface().CmdBeginQuery(*GetImpl(), *queryPoolImpl, offset);
}

void CommandBufferVal::EndQuery(const QueryPool& queryPool, uint32_t offset) {
    const QueryPoolVal& queryPoolVal = (const QueryPoolVal&)queryPool;

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdEndQuery: the command buffer must be in the recording state");

    if (!queryPoolVal.IsImported()) {
        RETURN_ON_FAILURE(&m_Device, offset < queryPoolVal.GetQueryNum(), ReturnVoid(), "CmdEndQuery: 'offset = %u' is out of range", offset);

        ValidationCommandUseQuery& validationCommand = AllocateValidationCommand<ValidationCommandUseQuery>();
        validationCommand.type = ValidationCommandType::END_QUERY;
        validationCommand.queryPool = const_cast<QueryPool*>(&queryPool);
        validationCommand.queryPoolOffset = offset;
    }

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);

    GetCoreInterface().CmdEndQuery(*GetImpl(), *queryPoolImpl, offset);
}

void CommandBufferVal::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdCopyQueries: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdCopyQueries: must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    const QueryPoolVal& queryPoolVal = (const QueryPoolVal&)queryPool;
    if (!queryPoolVal.IsImported())
        RETURN_ON_FAILURE(&m_Device, offset + num <= queryPoolVal.GetQueryNum(), ReturnVoid(), "CmdCopyQueries: 'offset + num =  %u' is out of range", offset + num);

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);
    Buffer* dstBufferImpl = NRI_GET_IMPL(Buffer, &dstBuffer);

    GetCoreInterface().CmdCopyQueries(*GetImpl(), *queryPoolImpl, offset, num, *dstBufferImpl, dstOffset);
}

void CommandBufferVal::ResetQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdResetQueries: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdResetQueries: must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    const QueryPoolVal& queryPoolVal = (const QueryPoolVal&)queryPool;
    if (!queryPoolVal.IsImported()) {
        RETURN_ON_FAILURE(&m_Device, offset + num <= queryPoolVal.GetQueryNum(), ReturnVoid(), "CmdResetQueries: 'offset + num = %u' is out of range", offset + num);

        ValidationCommandResetQuery& validationCommand = AllocateValidationCommand<ValidationCommandResetQuery>();
        validationCommand.type = ValidationCommandType::RESET_QUERY;
        validationCommand.queryPool = const_cast<QueryPool*>(&queryPool);
        validationCommand.queryPoolOffset = offset;
        validationCommand.queryNum = num;
    }

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);

    GetCoreInterface().CmdResetQueries(*GetImpl(), *queryPoolImpl, offset, num);
}

void CommandBufferVal::BeginAnnotation(const char* name) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdBeginAnnotation: the command buffer must be in the recording state");

    m_AnnotationStack++;
    GetCoreInterface().CmdBeginAnnotation(*GetImpl(), name);
}

void CommandBufferVal::EndAnnotation() {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdEndAnnotation: the command buffer must be in the recording state");

    GetCoreInterface().CmdEndAnnotation(*GetImpl());
    m_AnnotationStack--;
}

void CommandBufferVal::Destroy() {
    GetCoreInterface().DestroyCommandBuffer(*GetImpl());
    Deallocate(m_Device.GetStdAllocator(), this);
}

void CommandBufferVal::BuildTopLevelAccelerationStructure(
    uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset
) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdBuildTopLevelAccelerationStructure: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdBuildTopLevelAccelerationStructure: must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    BufferVal& bufferVal = (BufferVal&)buffer;
    BufferVal& scratchVal = (BufferVal&)scratch;

    RETURN_ON_FAILURE(
        &m_Device, bufferOffset < bufferVal.GetDesc().size, ReturnVoid(), "CmdBuildTopLevelAccelerationStructure: 'bufferOffset = %llu' is out of bounds", bufferOffset
    );

    RETURN_ON_FAILURE(
        &m_Device, scratchOffset < scratchVal.GetDesc().size, ReturnVoid(), "CmdBuildTopLevelAccelerationStructure: 'scratchOffset = %llu' is out of bounds", scratchOffset
    );

    AccelerationStructure& dstImpl = *NRI_GET_IMPL(AccelerationStructure, &dst);
    Buffer& scratchImpl = *NRI_GET_IMPL(Buffer, &scratch);
    Buffer& bufferImpl = *NRI_GET_IMPL(Buffer, &buffer);

    m_RayTracingAPI.CmdBuildTopLevelAccelerationStructure(*GetImpl(), instanceNum, bufferImpl, bufferOffset, flags, dstImpl, scratchImpl, scratchOffset);
}

void CommandBufferVal::BuildBottomLevelAccelerationStructure(
    uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset
) {
    BufferVal& scratchVal = (BufferVal&)scratch;

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdBuildBottomLevelAccelerationStructure: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdBuildBottomLevelAccelerationStructure: must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, geometryObjects != nullptr, ReturnVoid(), "CmdBuildBottomLevelAccelerationStructure: 'geometryObjects' is NULL");
    RETURN_ON_FAILURE(
        &m_Device, scratchOffset < scratchVal.GetDesc().size, ReturnVoid(), "CmdBuildBottomLevelAccelerationStructure: 'scratchOffset = %" PRIu64 "' is out of bounds",
        scratchOffset
    );

    AccelerationStructure& dstImpl = *NRI_GET_IMPL(AccelerationStructure, &dst);
    Buffer& scratchImpl = *NRI_GET_IMPL(Buffer, &scratch);

    Vector<GeometryObject> objectImplArray(geometryObjectNum, m_Device.GetStdAllocator());
    ConvertGeometryObjectsVal(objectImplArray.data(), geometryObjects, geometryObjectNum);

    m_RayTracingAPI.CmdBuildBottomLevelAccelerationStructure(*GetImpl(), geometryObjectNum, objectImplArray.data(), flags, dstImpl, scratchImpl, scratchOffset);
}

void CommandBufferVal::UpdateTopLevelAccelerationStructure(
    uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, AccelerationStructure& src,
    Buffer& scratch, uint64_t scratchOffset
) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdUpdateTopLevelAccelerationStructure: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdUpdateTopLevelAccelerationStructure: must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    BufferVal& bufferVal = (BufferVal&)buffer;
    BufferVal& scratchVal = (BufferVal&)scratch;

    RETURN_ON_FAILURE(
        &m_Device, bufferOffset < bufferVal.GetDesc().size, ReturnVoid(), "CmdUpdateTopLevelAccelerationStructure: 'bufferOffset = %" PRIu64 "' is out of bounds", bufferOffset
    );

    RETURN_ON_FAILURE(
        &m_Device, scratchOffset < scratchVal.GetDesc().size, ReturnVoid(), "CmdUpdateTopLevelAccelerationStructure: 'scratchOffset = %" PRIu64 "' is out of bounds", scratchOffset
    );

    AccelerationStructure& dstImpl = *NRI_GET_IMPL(AccelerationStructure, &dst);
    AccelerationStructure& srcImpl = *NRI_GET_IMPL(AccelerationStructure, &src);
    Buffer& scratchImpl = *NRI_GET_IMPL(Buffer, &scratch);
    Buffer& bufferImpl = *NRI_GET_IMPL(Buffer, &buffer);

    m_RayTracingAPI.CmdUpdateTopLevelAccelerationStructure(*GetImpl(), instanceNum, bufferImpl, bufferOffset, flags, dstImpl, srcImpl, scratchImpl, scratchOffset);
}

void CommandBufferVal::UpdateBottomLevelAccelerationStructure(
    uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, AccelerationStructure& src,
    Buffer& scratch, uint64_t scratchOffset
) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdUpdateBottomLevelAccelerationStructure: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdUpdateBottomLevelAccelerationStructure: must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, geometryObjects != nullptr, ReturnVoid(), "CmdUpdateBottomLevelAccelerationStructure: 'geometryObjects' is NULL");

    BufferVal& scratchVal = (BufferVal&)scratch;

    RETURN_ON_FAILURE(
        &m_Device, scratchOffset < scratchVal.GetDesc().size, ReturnVoid(), "CmdUpdateBottomLevelAccelerationStructure: 'scratchOffset = %" PRIu64 "' is out of bounds",
        scratchOffset
    );

    AccelerationStructure& dstImpl = *NRI_GET_IMPL(AccelerationStructure, &dst);
    AccelerationStructure& srcImpl = *NRI_GET_IMPL(AccelerationStructure, &src);
    Buffer& scratchImpl = *NRI_GET_IMPL(Buffer, &scratch);

    Vector<GeometryObject> objectImplArray(geometryObjectNum, m_Device.GetStdAllocator());
    ConvertGeometryObjectsVal(objectImplArray.data(), geometryObjects, geometryObjectNum);

    m_RayTracingAPI.CmdUpdateBottomLevelAccelerationStructure(*GetImpl(), geometryObjectNum, objectImplArray.data(), flags, dstImpl, srcImpl, scratchImpl, scratchOffset);
}

void CommandBufferVal::CopyAccelerationStructure(AccelerationStructure& dst, AccelerationStructure& src, CopyMode copyMode) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdCopyAccelerationStructure: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdCopyAccelerationStructure: must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, copyMode < CopyMode::MAX_NUM, ReturnVoid(), "CmdCopyAccelerationStructure: 'copyMode' is invalid");

    AccelerationStructure& dstImpl = *NRI_GET_IMPL(AccelerationStructure, &dst);
    AccelerationStructure& srcImpl = *NRI_GET_IMPL(AccelerationStructure, &src);

    m_RayTracingAPI.CmdCopyAccelerationStructure(*GetImpl(), dstImpl, srcImpl, copyMode);
}

void CommandBufferVal::WriteAccelerationStructureSize(
    const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryOffset
) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdWriteAccelerationStructureSize: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdWriteAccelerationStructureSize: must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, accelerationStructures != nullptr, ReturnVoid(), "CmdWriteAccelerationStructureSize: 'accelerationStructures' is NULL");

    AccelerationStructure** accelerationStructureArray = STACK_ALLOC(AccelerationStructure*, accelerationStructureNum);
    for (uint32_t i = 0; i < accelerationStructureNum; i++) {
        RETURN_ON_FAILURE(&m_Device, accelerationStructures[i] != nullptr, ReturnVoid(), "CmdWriteAccelerationStructureSize: 'accelerationStructures[%u]' is NULL", i);

        accelerationStructureArray[i] = NRI_GET_IMPL(AccelerationStructure, accelerationStructures[i]);
    }

    QueryPool& queryPoolImpl = *NRI_GET_IMPL(QueryPool, &queryPool);

    m_RayTracingAPI.CmdWriteAccelerationStructureSize(*GetImpl(), accelerationStructures, accelerationStructureNum, queryPoolImpl, queryOffset);
}

void CommandBufferVal::DispatchRays(const DispatchRaysDesc& dispatchRaysDesc) {
    const uint64_t SBTAlignment = m_Device.GetDesc().rayTracingShaderTableAligment;

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "CmdDispatchRays: the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "CmdDispatchRays: must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.raygenShader.buffer != nullptr, ReturnVoid(), "CmdDispatchRays: 'dispatchRaysDesc.raygenShader.buffer' is NULL");
    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.raygenShader.size != 0, ReturnVoid(), "CmdDispatchRays: 'dispatchRaysDesc.raygenShader.size' is 0");
    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.raygenShader.offset % SBTAlignment == 0, ReturnVoid(), "CmdDispatchRays: 'dispatchRaysDesc.raygenShader.offset' is misaligned");
    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.missShaders.offset % SBTAlignment == 0, ReturnVoid(), "CmdDispatchRays: 'dispatchRaysDesc.missShaders.offset' is misaligned");

    RETURN_ON_FAILURE(
        &m_Device, dispatchRaysDesc.hitShaderGroups.offset % SBTAlignment == 0, ReturnVoid(), "CmdDispatchRays: 'dispatchRaysDesc.hitShaderGroups.offset' is misaligned"
    );

    RETURN_ON_FAILURE(
        &m_Device, dispatchRaysDesc.callableShaders.offset % SBTAlignment == 0, ReturnVoid(), "CmdDispatchRays: 'dispatchRaysDesc.callableShaders.offset' is misaligned"
    );

    auto dispatchRaysDescImpl = dispatchRaysDesc;
    dispatchRaysDescImpl.raygenShader.buffer = NRI_GET_IMPL(Buffer, dispatchRaysDesc.raygenShader.buffer);
    dispatchRaysDescImpl.missShaders.buffer = NRI_GET_IMPL(Buffer, dispatchRaysDesc.missShaders.buffer);
    dispatchRaysDescImpl.hitShaderGroups.buffer = NRI_GET_IMPL(Buffer, dispatchRaysDesc.hitShaderGroups.buffer);
    dispatchRaysDescImpl.callableShaders.buffer = NRI_GET_IMPL(Buffer, dispatchRaysDesc.callableShaders.buffer);

    m_RayTracingAPI.CmdDispatchRays(*GetImpl(), dispatchRaysDescImpl);
}

void CommandBufferVal::DispatchMeshTasks(uint32_t x, uint32_t y, uint32_t z) {
    m_MeshShaderAPI.CmdDispatchMeshTasks(*GetImpl(), x, y, z);
}

template <typename Command>
Command& CommandBufferVal::AllocateValidationCommand() {
    const size_t commandSize = sizeof(Command);
    const size_t newSize = m_ValidationCommands.size() + commandSize;
    const size_t capacity = m_ValidationCommands.capacity();

    if (newSize > capacity)
        m_ValidationCommands.reserve(std::max(capacity + (capacity >> 1), newSize));

    const size_t offset = m_ValidationCommands.size();
    m_ValidationCommands.resize(newSize);

    return *(Command*)(m_ValidationCommands.data() + offset);
}

#include "CommandBufferVal.hpp"
