// © 2021 NVIDIA Corporation

#include "SharedExternal.h"
#include "SharedVal.h"

#include "AccelerationStructureVal.h"
#include "BufferVal.h"
#include "CommandBufferVal.h"
#include "DescriptorPoolVal.h"
#include "DescriptorVal.h"
#include "DeviceVal.h"
#include "PipelineLayoutVal.h"
#include "PipelineVal.h"
#include "QueryPoolVal.h"
#include "TextureVal.h"

using namespace nri;

void ConvertGeometryObjectsVal(GeometryObject* destObjects, const GeometryObject* sourceObjects, uint32_t objectNum);

static bool ValidateBufferBarrierDesc(const DeviceVal& device, uint32_t i, const BufferBarrierDesc& bufferBarrierDesc) {
    const BufferVal& bufferVal = *(const BufferVal*)bufferBarrierDesc.buffer;

    RETURN_ON_FAILURE(&device, bufferBarrierDesc.buffer != nullptr, false, "'bufferBarrierDesc.buffers[%u].buffer' is NULL", i);
    RETURN_ON_FAILURE(&device, IsAccessMaskSupported(bufferVal.GetDesc().usageMask, bufferBarrierDesc.before.access), false,
        "'bufferBarrierDesc.buffers[%u].before' is not supported by the usage mask of the buffer ('%s')", i, bufferVal.GetDebugName());
    RETURN_ON_FAILURE(&device, IsAccessMaskSupported(bufferVal.GetDesc().usageMask, bufferBarrierDesc.after.access), false,
        "'bufferBarrierDesc.buffers[%u].after' is not supported by the usage mask of the buffer ('%s')", i, bufferVal.GetDebugName());

    return true;
}

static bool ValidateTextureBarrierDesc(const DeviceVal& device, uint32_t i, const TextureBarrierDesc& textureBarrierDesc) {
    const TextureVal& textureVal = *(const TextureVal*)textureBarrierDesc.texture;

    RETURN_ON_FAILURE(&device, textureBarrierDesc.texture != nullptr, false, "'bufferBarrierDesc.textures[%u].texture' is NULL", i);
    RETURN_ON_FAILURE(&device, IsAccessMaskSupported(textureVal.GetDesc().usageMask, textureBarrierDesc.before.access), false,
        "'bufferBarrierDesc.textures[%u].before' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName());
    RETURN_ON_FAILURE(&device, IsAccessMaskSupported(textureVal.GetDesc().usageMask, textureBarrierDesc.after.access), false,
        "'bufferBarrierDesc.textures[%u].after' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName());
    RETURN_ON_FAILURE(&device, IsTextureLayoutSupported(textureVal.GetDesc().usageMask, textureBarrierDesc.before.layout), false,
        "'bufferBarrierDesc.textures[%u].prevLayout' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName());
    RETURN_ON_FAILURE(&device, IsTextureLayoutSupported(textureVal.GetDesc().usageMask, textureBarrierDesc.after.layout), false,
        "'bufferBarrierDesc.textures[%u].nextLayout' is not supported by the usage mask of the texture ('%s')", i, textureVal.GetDebugName());

    return true;
}

void CommandBufferVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetCommandBufferDebugName(*GetImpl(), name);
}

Result CommandBufferVal::Begin(const DescriptorPool* descriptorPool) {
    RETURN_ON_FAILURE(&m_Device, !m_IsRecordingStarted, Result::FAILURE, "already in the recording state");

    DescriptorPool* descriptorPoolImpl = NRI_GET_IMPL(DescriptorPool, descriptorPool);

    Result result = GetCoreInterface().BeginCommandBuffer(*GetImpl(), descriptorPoolImpl);
    if (result == Result::SUCCESS)
        m_IsRecordingStarted = true;

    m_ValidationCommands.clear();
    m_Pipeline = nullptr;

    ResetAttachments();

    return result;
}

Result CommandBufferVal::End() {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, Result::FAILURE, "not in the recording state");

    if (m_AnnotationStack > 0)
        REPORT_ERROR(&m_Device, "'CmdBeginAnnotation' is called more times than 'CmdEndAnnotation'");
    else if (m_AnnotationStack < 0)
        REPORT_ERROR(&m_Device, "'CmdEndAnnotation' is called more times than 'CmdBeginAnnotation'");

    Result result = GetCoreInterface().EndCommandBuffer(*GetImpl());
    if (result == Result::SUCCESS)
        m_IsRecordingStarted = m_IsWrapped;

    return result;
}

void CommandBufferVal::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    if (!viewportNum)
        return;

    RETURN_ON_FAILURE(&m_Device, viewports, ReturnVoid(), "'viewports' is NULL");

    GetCoreInterface().CmdSetViewports(*GetImpl(), viewports, viewportNum);
}

void CommandBufferVal::SetScissors(const Rect* rects, uint32_t rectNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    if (!rectNum)
        return;

    RETURN_ON_FAILURE(&m_Device, rects, ReturnVoid(), "'rects' is NULL");

    GetCoreInterface().CmdSetScissors(*GetImpl(), rects, rectNum);
}

void CommandBufferVal::SetDepthBounds(float boundsMin, float boundsMax) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_Device.GetDesc().isDepthBoundsTestSupported, ReturnVoid(), "DeviceDesc::isDepthBoundsTestSupported = false");

    GetCoreInterface().CmdSetDepthBounds(*GetImpl(), boundsMin, boundsMax);
}

void CommandBufferVal::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterface().CmdSetStencilReference(*GetImpl(), frontRef, backRef);
}

void CommandBufferVal::SetSampleLocations(const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_Device.GetDesc().sampleLocationsTier != 0, ReturnVoid(), "DeviceDesc::sampleLocationsTier = 0");

    GetCoreInterface().CmdSetSampleLocations(*GetImpl(), locations, locationNum, sampleNum);
}

void CommandBufferVal::SetBlendConstants(const Color32f& color) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterface().CmdSetBlendConstants(*GetImpl(), color);
}

void CommandBufferVal::SetShadingRate(const ShadingRateDesc& shadingRateDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_Device.GetDesc().shadingRateTier, ReturnVoid(), "DeviceDesc::shadingRateTier = 0");

    GetCoreInterface().CmdSetShadingRate(*GetImpl(), shadingRateDesc);
}

void CommandBufferVal::SetDepthBias(const DepthBiasDesc& depthBiasDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_Device.GetDesc().isDynamicDepthBiasSupported, ReturnVoid(), "DeviceDesc::isDynamicDepthBiasSupported = false");

    GetCoreInterface().CmdSetDepthBias(*GetImpl(), depthBiasDesc);
}

void CommandBufferVal::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    for (uint32_t i = 0; i < clearDescNum; i++) {
        RETURN_ON_FAILURE(&m_Device, (clearDescs[i].planes & (PlaneBits::COLOR | PlaneBits::DEPTH | PlaneBits::STENCIL)) != 0, ReturnVoid(), "'clearDesc[%u].planes' is not COLOR, DEPTH or STENCIL", i);

        if (clearDescs[i].planes & PlaneBits::COLOR) {
            RETURN_ON_FAILURE(&m_Device, clearDescs[i].colorAttachmentIndex < deviceDesc.colorAttachmentMaxNum, ReturnVoid(), "'clearDesc[%u].colorAttachmentIndex = %u' is out of bounds", i, clearDescs[i].colorAttachmentIndex);
            RETURN_ON_FAILURE(&m_Device, m_RenderTargets[clearDescs[i].colorAttachmentIndex], ReturnVoid(), "'clearDesc[%u].colorAttachmentIndex = %u' references a NULL COLOR attachment", i, clearDescs[i].colorAttachmentIndex);
        }

        if (clearDescs[i].planes & (PlaneBits::DEPTH | PlaneBits::STENCIL))
            RETURN_ON_FAILURE(&m_Device, m_DepthStencil, ReturnVoid(), "DEPTH_STENCIL attachment is NULL", i);

        if (clearDescs[i].colorAttachmentIndex != 0)
            RETURN_ON_FAILURE(&m_Device, (clearDescs[i].planes & PlaneBits::COLOR), ReturnVoid(), "'clearDesc[%u].planes' is not COLOR, but `colorAttachmentIndex != 0`", i);
    }

    GetCoreInterface().CmdClearAttachments(*GetImpl(), clearDescs, clearDescNum, rects, rectNum);
}

void CommandBufferVal::ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, clearDesc.storageBuffer, ReturnVoid(), "'clearDesc.storageBuffer' is NULL");

    auto clearDescImpl = clearDesc;
    clearDescImpl.storageBuffer = NRI_GET_IMPL(Descriptor, clearDesc.storageBuffer);

    GetCoreInterface().CmdClearStorageBuffer(*GetImpl(), clearDescImpl);
}

void CommandBufferVal::ClearStorageTexture(const ClearStorageTextureDesc& clearDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, clearDesc.storageTexture, ReturnVoid(), "'clearDesc.storageTexture' is NULL");

    auto clearDescImpl = clearDesc;
    clearDescImpl.storageTexture = NRI_GET_IMPL(Descriptor, clearDesc.storageTexture);

    GetCoreInterface().CmdClearStorageTexture(*GetImpl(), clearDescImpl);
}

void CommandBufferVal::BeginRendering(const AttachmentsDesc& attachmentsDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "'CmdBeginRendering' has been already called");

    Descriptor** colors = StackAlloc(Descriptor*, attachmentsDesc.colorNum);
    for (uint32_t i = 0; i < attachmentsDesc.colorNum; i++)
        colors[i] = NRI_GET_IMPL(Descriptor, attachmentsDesc.colors[i]);

    AttachmentsDesc attachmentsDescImpl = {};
    attachmentsDescImpl.depthStencil = NRI_GET_IMPL(Descriptor, attachmentsDesc.depthStencil);
    attachmentsDescImpl.shadingRate = NRI_GET_IMPL(Descriptor, attachmentsDesc.shadingRate);
    attachmentsDescImpl.colors = colors;
    attachmentsDescImpl.colorNum = attachmentsDesc.colorNum;

    m_IsRenderPass = true;
    m_RenderTargetNum = attachmentsDesc.colors ? attachmentsDesc.colorNum : 0;

    size_t i = 0;
    for (; i < m_RenderTargetNum; i++)
        m_RenderTargets[i] = (DescriptorVal*)attachmentsDesc.colors[i];
    for (; i < m_RenderTargets.size(); i++)
        m_RenderTargets[i] = nullptr;

    if (attachmentsDesc.depthStencil)
        m_DepthStencil = (DescriptorVal*)attachmentsDesc.depthStencil;
    else
        m_DepthStencil = nullptr;

    ValidateReadonlyDepthStencil();

    GetCoreInterface().CmdBeginRendering(*GetImpl(), attachmentsDescImpl);
}

void CommandBufferVal::EndRendering() {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "'CmdBeginRendering' has not been called");

    m_IsRenderPass = false;

    ResetAttachments();

    GetCoreInterface().CmdEndRendering(*GetImpl());
}

void CommandBufferVal::SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    Buffer** buffersImpl = StackAlloc(Buffer*, bufferNum);
    for (uint32_t i = 0; i < bufferNum; i++)
        buffersImpl[i] = NRI_GET_IMPL(Buffer, buffers[i]);

    GetCoreInterface().CmdSetVertexBuffers(*GetImpl(), baseSlot, bufferNum, buffersImpl, offsets);
}

void CommandBufferVal::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);

    GetCoreInterface().CmdSetIndexBuffer(*GetImpl(), *bufferImpl, offset, indexType);
}

void CommandBufferVal::SetPipelineLayout(const PipelineLayout& pipelineLayout) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    PipelineLayout* pipelineLayoutImpl = NRI_GET_IMPL(PipelineLayout, &pipelineLayout);

    GetCoreInterface().CmdSetPipelineLayout(*GetImpl(), *pipelineLayoutImpl);
}

void CommandBufferVal::SetPipeline(const Pipeline& pipeline) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    Pipeline* pipelineImpl = NRI_GET_IMPL(Pipeline, &pipeline);

    m_Pipeline = (PipelineVal*)&pipeline;
    ValidateReadonlyDepthStencil();

    GetCoreInterface().CmdSetPipeline(*GetImpl(), *pipelineImpl);
}

void CommandBufferVal::SetDescriptorPool(const DescriptorPool& descriptorPool) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    DescriptorPool* descriptorPoolImpl = NRI_GET_IMPL(DescriptorPool, &descriptorPool);

    GetCoreInterface().CmdSetDescriptorPool(*GetImpl(), *descriptorPoolImpl);
}

void CommandBufferVal::SetDescriptorSet(uint32_t setIndex, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    DescriptorSet* descriptorSetImpl = NRI_GET_IMPL(DescriptorSet, &descriptorSet);

    GetCoreInterface().CmdSetDescriptorSet(*GetImpl(), setIndex, *descriptorSetImpl, dynamicConstantBufferOffsets);
}

void CommandBufferVal::SetConstants(uint32_t pushConstantIndex, const void* data, uint32_t size) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterface().CmdSetConstants(*GetImpl(), pushConstantIndex, data, size);
}

void CommandBufferVal::Draw(const DrawDesc& drawDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");

    GetCoreInterface().CmdDraw(*GetImpl(), drawDesc);
}

void CommandBufferVal::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");

    GetCoreInterface().CmdDrawIndexed(*GetImpl(), drawIndexedDesc);
}

void CommandBufferVal::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, !countBuffer || deviceDesc.isDrawIndirectCountSupported, ReturnVoid(), "'countBuffer' is not supported");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    Buffer* countBufferImpl = NRI_GET_IMPL(Buffer, countBuffer);

    GetCoreInterface().CmdDrawIndirect(*GetImpl(), *bufferImpl, offset, drawNum, stride, countBufferImpl, countBufferOffset);
}

void CommandBufferVal::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, !countBuffer || deviceDesc.isDrawIndirectCountSupported, ReturnVoid(), "'countBuffer' is not supported");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    Buffer* countBufferImpl = NRI_GET_IMPL(Buffer, countBuffer);

    GetCoreInterface().CmdDrawIndexedIndirect(*GetImpl(), *bufferImpl, offset, drawNum, stride, countBufferImpl, countBufferOffset);
}

void CommandBufferVal::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    if (size == WHOLE_SIZE) {
        const BufferDesc& dstDesc = ((BufferVal&)dstBuffer).GetDesc();
        const BufferDesc& srcDesc = ((BufferVal&)srcBuffer).GetDesc();

        if (dstDesc.size != srcDesc.size)
            REPORT_WARNING(&m_Device, "WHOLE_SIZE is used but 'dstBuffer' and 'srcBuffer' have different sizes");
    }

    Buffer* dstBufferImpl = NRI_GET_IMPL(Buffer, &dstBuffer);
    Buffer* srcBufferImpl = NRI_GET_IMPL(Buffer, &srcBuffer);

    GetCoreInterface().CmdCopyBuffer(*GetImpl(), *dstBufferImpl, dstOffset, *srcBufferImpl, srcOffset, size);
}

void CommandBufferVal::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, (!dstRegionDesc && !srcRegionDesc) || (dstRegionDesc && srcRegionDesc), ReturnVoid(), "'dstRegionDesc' and 'srcRegionDesc' must be valid pointers or both NULL");

    Texture* dstTextureImpl = NRI_GET_IMPL(Texture, &dstTexture);
    Texture* srcTextureImpl = NRI_GET_IMPL(Texture, &srcTexture);

    GetCoreInterface().CmdCopyTexture(*GetImpl(), *dstTextureImpl, dstRegionDesc, *srcTextureImpl, srcRegionDesc);
}

void CommandBufferVal::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Texture* dstTextureImpl = NRI_GET_IMPL(Texture, &dstTexture);
    Buffer* srcBufferImpl = NRI_GET_IMPL(Buffer, &srcBuffer);

    GetCoreInterface().CmdUploadBufferToTexture(*GetImpl(), *dstTextureImpl, dstRegionDesc, *srcBufferImpl, srcDataLayoutDesc);
}

void CommandBufferVal::ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    Buffer* dstBufferImpl = NRI_GET_IMPL(Buffer, &dstBuffer);
    Texture* srcTextureImpl = NRI_GET_IMPL(Texture, &srcTexture);

    GetCoreInterface().CmdReadbackTextureToBuffer(*GetImpl(), *dstBufferImpl, dstDataLayoutDesc, *srcTextureImpl, srcRegionDesc);
}

void CommandBufferVal::Dispatch(const DispatchDesc& dispatchDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    GetCoreInterface().CmdDispatch(*GetImpl(), dispatchDesc);
}

void CommandBufferVal::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    const BufferDesc& bufferDesc = ((BufferVal&)buffer).GetDesc();
    RETURN_ON_FAILURE(&m_Device, offset < bufferDesc.size, ReturnVoid(), "offset is greater than the buffer size");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    GetCoreInterface().CmdDispatchIndirect(*GetImpl(), *bufferImpl, offset);
}

void CommandBufferVal::Barrier(const BarrierGroupDesc& barrierGroupDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    for (uint32_t i = 0; i < barrierGroupDesc.bufferNum; i++) {
        if (!ValidateBufferBarrierDesc(m_Device, i, barrierGroupDesc.buffers[i]))
            return;
    }

    for (uint32_t i = 0; i < barrierGroupDesc.textureNum; i++) {
        if (!ValidateTextureBarrierDesc(m_Device, i, barrierGroupDesc.textures[i]))
            return;
    }

    BufferBarrierDesc* buffers = StackAlloc(BufferBarrierDesc, barrierGroupDesc.bufferNum);
    memcpy(buffers, barrierGroupDesc.buffers, sizeof(BufferBarrierDesc) * barrierGroupDesc.bufferNum);
    for (uint32_t i = 0; i < barrierGroupDesc.bufferNum; i++)
        buffers[i].buffer = NRI_GET_IMPL(Buffer, barrierGroupDesc.buffers[i].buffer);

    TextureBarrierDesc* textures = StackAlloc(TextureBarrierDesc, barrierGroupDesc.textureNum);
    memcpy(textures, barrierGroupDesc.textures, sizeof(TextureBarrierDesc) * barrierGroupDesc.textureNum);
    for (uint32_t i = 0; i < barrierGroupDesc.textureNum; i++)
        textures[i].texture = NRI_GET_IMPL(Texture, barrierGroupDesc.textures[i].texture);

    BarrierGroupDesc barrierGroupDescImpl = barrierGroupDesc;
    barrierGroupDescImpl.buffers = buffers;
    barrierGroupDescImpl.textures = textures;

    GetCoreInterface().CmdBarrier(*GetImpl(), barrierGroupDescImpl);
}

void CommandBufferVal::BeginQuery(const QueryPool& queryPool, uint32_t offset) {
    const QueryPoolVal& queryPoolVal = (const QueryPoolVal&)queryPool;

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, queryPoolVal.GetQueryType() != QueryType::TIMESTAMP, ReturnVoid(), "'BeginQuery' is not supported for timestamp queries");

    if (!queryPoolVal.IsImported()) {
        RETURN_ON_FAILURE(&m_Device, offset < queryPoolVal.GetQueryNum(), ReturnVoid(), "'offset = %u' is out of range", offset);

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

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    if (!queryPoolVal.IsImported()) {
        RETURN_ON_FAILURE(&m_Device, offset < queryPoolVal.GetQueryNum(), ReturnVoid(), "'offset = %u' is out of range", offset);

        ValidationCommandUseQuery& validationCommand = AllocateValidationCommand<ValidationCommandUseQuery>();
        validationCommand.type = ValidationCommandType::END_QUERY;
        validationCommand.queryPool = const_cast<QueryPool*>(&queryPool);
        validationCommand.queryPoolOffset = offset;
    }

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);

    GetCoreInterface().CmdEndQuery(*GetImpl(), *queryPoolImpl, offset);
}

void CommandBufferVal::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    const QueryPoolVal& queryPoolVal = (const QueryPoolVal&)queryPool;
    if (!queryPoolVal.IsImported())
        RETURN_ON_FAILURE(&m_Device, offset + num <= queryPoolVal.GetQueryNum(), ReturnVoid(), "'offset + num =  %u' is out of range", offset + num);

    QueryPool* queryPoolImpl = NRI_GET_IMPL(QueryPool, &queryPool);
    Buffer* dstBufferImpl = NRI_GET_IMPL(Buffer, &dstBuffer);

    GetCoreInterface().CmdCopyQueries(*GetImpl(), *queryPoolImpl, offset, num, *dstBufferImpl, dstOffset);
}

void CommandBufferVal::ResetQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    const QueryPoolVal& queryPoolVal = (const QueryPoolVal&)queryPool;
    if (!queryPoolVal.IsImported()) {
        RETURN_ON_FAILURE(&m_Device, offset + num <= queryPoolVal.GetQueryNum(), ReturnVoid(), "'offset + num = %u' is out of range", offset + num);

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
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    m_AnnotationStack++;
    GetCoreInterface().CmdBeginAnnotation(*GetImpl(), name);
}

void CommandBufferVal::EndAnnotation() {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");

    GetCoreInterface().CmdEndAnnotation(*GetImpl());
    m_AnnotationStack--;
}

void CommandBufferVal::BuildTopLevelAccelerationStructure(
    uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    BufferVal& bufferVal = (BufferVal&)buffer;
    BufferVal& scratchVal = (BufferVal&)scratch;

    RETURN_ON_FAILURE(&m_Device, bufferOffset < bufferVal.GetDesc().size, ReturnVoid(), "'bufferOffset = %llu' is out of bounds", bufferOffset);
    RETURN_ON_FAILURE(&m_Device, scratchOffset < scratchVal.GetDesc().size, ReturnVoid(), "'scratchOffset = %llu' is out of bounds", scratchOffset);

    AccelerationStructure& dstImpl = *NRI_GET_IMPL(AccelerationStructure, &dst);
    Buffer& scratchImpl = *NRI_GET_IMPL(Buffer, &scratch);
    Buffer& bufferImpl = *NRI_GET_IMPL(Buffer, &buffer);

    GetRayTracingInterface().CmdBuildTopLevelAccelerationStructure(*GetImpl(), instanceNum, bufferImpl, bufferOffset, flags, dstImpl, scratchImpl, scratchOffset);
}

void CommandBufferVal::BuildBottomLevelAccelerationStructure(
    uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset) {
    BufferVal& scratchVal = (BufferVal&)scratch;

    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, geometryObjects, ReturnVoid(), "'geometryObjects' is NULL");
    RETURN_ON_FAILURE(&m_Device, scratchOffset < scratchVal.GetDesc().size, ReturnVoid(), "'scratchOffset = %llu' is out of bounds", scratchOffset);

    AccelerationStructure& dstImpl = *NRI_GET_IMPL(AccelerationStructure, &dst);
    Buffer& scratchImpl = *NRI_GET_IMPL(Buffer, &scratch);

    Scratch<GeometryObject> objectImplArray = AllocateScratch(m_Device, GeometryObject, geometryObjectNum);
    ConvertGeometryObjectsVal(objectImplArray, geometryObjects, geometryObjectNum);

    GetRayTracingInterface().CmdBuildBottomLevelAccelerationStructure(*GetImpl(), geometryObjectNum, objectImplArray, flags, dstImpl, scratchImpl, scratchOffset);
}

void CommandBufferVal::UpdateTopLevelAccelerationStructure(uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags,
    AccelerationStructure& dst, const AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");

    BufferVal& bufferVal = (BufferVal&)buffer;
    BufferVal& scratchVal = (BufferVal&)scratch;

    RETURN_ON_FAILURE(&m_Device, bufferOffset < bufferVal.GetDesc().size, ReturnVoid(), "'bufferOffset = %llu' is out of bounds", bufferOffset);
    RETURN_ON_FAILURE(&m_Device, scratchOffset < scratchVal.GetDesc().size, ReturnVoid(), "'scratchOffset = %llu' is out of bounds", scratchOffset);

    AccelerationStructure& dstImpl = *NRI_GET_IMPL(AccelerationStructure, &dst);
    AccelerationStructure& srcImpl = *NRI_GET_IMPL(AccelerationStructure, &src);
    Buffer& scratchImpl = *NRI_GET_IMPL(Buffer, &scratch);
    Buffer& bufferImpl = *NRI_GET_IMPL(Buffer, &buffer);

    GetRayTracingInterface().CmdUpdateTopLevelAccelerationStructure(*GetImpl(), instanceNum, bufferImpl, bufferOffset, flags, dstImpl, srcImpl, scratchImpl, scratchOffset);
}

void CommandBufferVal::UpdateBottomLevelAccelerationStructure(uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags,
    AccelerationStructure& dst, const AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, geometryObjects, ReturnVoid(), "'geometryObjects' is NULL");

    BufferVal& scratchVal = (BufferVal&)scratch;

    RETURN_ON_FAILURE(&m_Device, scratchOffset < scratchVal.GetDesc().size, ReturnVoid(), "'scratchOffset = %llu' is out of bounds", scratchOffset);

    AccelerationStructure& dstImpl = *NRI_GET_IMPL(AccelerationStructure, &dst);
    AccelerationStructure& srcImpl = *NRI_GET_IMPL(AccelerationStructure, &src);
    Buffer& scratchImpl = *NRI_GET_IMPL(Buffer, &scratch);

    Scratch<GeometryObject> objectImplArray = AllocateScratch(m_Device, GeometryObject, geometryObjectNum);
    ConvertGeometryObjectsVal(objectImplArray, geometryObjects, geometryObjectNum);

    GetRayTracingInterface().CmdUpdateBottomLevelAccelerationStructure(*GetImpl(), geometryObjectNum, objectImplArray, flags, dstImpl, srcImpl, scratchImpl, scratchOffset);
}

void CommandBufferVal::CopyAccelerationStructure(AccelerationStructure& dst, const AccelerationStructure& src, CopyMode copyMode) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, copyMode < CopyMode::MAX_NUM, ReturnVoid(), "'copyMode' is invalid");

    AccelerationStructure& dstImpl = *NRI_GET_IMPL(AccelerationStructure, &dst);
    AccelerationStructure& srcImpl = *NRI_GET_IMPL(AccelerationStructure, &src);

    GetRayTracingInterface().CmdCopyAccelerationStructure(*GetImpl(), dstImpl, srcImpl, copyMode);
}

void CommandBufferVal::WriteAccelerationStructureSize(
    const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryOffset) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, accelerationStructures, ReturnVoid(), "'accelerationStructures' is NULL");

    AccelerationStructure** accelerationStructureArray = StackAlloc(AccelerationStructure*, accelerationStructureNum);
    for (uint32_t i = 0; i < accelerationStructureNum; i++) {
        RETURN_ON_FAILURE(&m_Device, accelerationStructures[i], ReturnVoid(), "'accelerationStructures[%u]' is NULL", i);

        accelerationStructureArray[i] = NRI_GET_IMPL(AccelerationStructure, accelerationStructures[i]);
    }

    QueryPool& queryPoolImpl = *NRI_GET_IMPL(QueryPool, &queryPool);

    GetRayTracingInterface().CmdWriteAccelerationStructureSize(*GetImpl(), accelerationStructures, accelerationStructureNum, queryPoolImpl, queryOffset);
}

void CommandBufferVal::DispatchRays(const DispatchRaysDesc& dispatchRaysDesc) {
    uint64_t align = m_Device.GetDesc().rayTracingShaderTableAlignment;
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, !m_IsRenderPass, ReturnVoid(), "must be called outside of 'CmdBeginRendering/CmdEndRendering'");
    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.raygenShader.buffer, ReturnVoid(), "'dispatchRaysDesc.raygenShader.buffer' is NULL");
    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.raygenShader.size != 0, ReturnVoid(), "'dispatchRaysDesc.raygenShader.size' is 0");
    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.raygenShader.offset % align == 0, ReturnVoid(), "'dispatchRaysDesc.raygenShader.offset' is misaligned");
    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.missShaders.offset % align == 0, ReturnVoid(), "'dispatchRaysDesc.missShaders.offset' is misaligned");
    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.hitShaderGroups.offset % align == 0, ReturnVoid(), "'dispatchRaysDesc.hitShaderGroups.offset' is misaligned");
    RETURN_ON_FAILURE(&m_Device, dispatchRaysDesc.callableShaders.offset % align == 0, ReturnVoid(), "'dispatchRaysDesc.callableShaders.offset' is misaligned");

    auto dispatchRaysDescImpl = dispatchRaysDesc;
    dispatchRaysDescImpl.raygenShader.buffer = NRI_GET_IMPL(Buffer, dispatchRaysDesc.raygenShader.buffer);
    dispatchRaysDescImpl.missShaders.buffer = NRI_GET_IMPL(Buffer, dispatchRaysDesc.missShaders.buffer);
    dispatchRaysDescImpl.hitShaderGroups.buffer = NRI_GET_IMPL(Buffer, dispatchRaysDesc.hitShaderGroups.buffer);
    dispatchRaysDescImpl.callableShaders.buffer = NRI_GET_IMPL(Buffer, dispatchRaysDesc.callableShaders.buffer);

    GetRayTracingInterface().CmdDispatchRays(*GetImpl(), dispatchRaysDescImpl);
}

void CommandBufferVal::DispatchRaysIndirect(const Buffer& buffer, uint64_t offset) {
    const BufferDesc& bufferDesc = ((BufferVal&)buffer).GetDesc();
    RETURN_ON_FAILURE(&m_Device, offset < bufferDesc.size, ReturnVoid(), "offset is greater than the buffer size");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    GetRayTracingInterface().CmdDispatchRaysIndirect(*GetImpl(), *bufferImpl, offset);
}

void CommandBufferVal::DrawMeshTasks(const DrawMeshTasksDesc& drawMeshTasksDesc) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");

    GetMeshShaderInterface().CmdDrawMeshTasks(*GetImpl(), drawMeshTasksDesc);
}

void CommandBufferVal::DrawMeshTasksIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    RETURN_ON_FAILURE(&m_Device, m_IsRecordingStarted, ReturnVoid(), "the command buffer must be in the recording state");
    RETURN_ON_FAILURE(&m_Device, m_IsRenderPass, ReturnVoid(), "must be called inside 'CmdBeginRendering/CmdEndRendering'");

    const BufferDesc& bufferDesc = ((BufferVal&)buffer).GetDesc();
    RETURN_ON_FAILURE(&m_Device, offset < bufferDesc.size, ReturnVoid(), "'offset' is greater than the buffer size");

    Buffer* bufferImpl = NRI_GET_IMPL(Buffer, &buffer);
    Buffer* countBufferImpl = NRI_GET_IMPL(Buffer, countBuffer);

    GetMeshShaderInterface().CmdDrawMeshTasksIndirect(*GetImpl(), *bufferImpl, offset, drawNum, stride, countBufferImpl, countBufferOffset);
}

void CommandBufferVal::ValidateReadonlyDepthStencil() {
    if (m_Pipeline && m_DepthStencil) {
        if (m_DepthStencil->IsDepthReadonly() && m_Pipeline->WritesToDepth())
            REPORT_WARNING(&m_Device, "Depth is read-only, but the pipeline writes to depth. Writing happens only in VK!");

        if (m_DepthStencil->IsStencilReadonly() && m_Pipeline->WritesToStencil())
            REPORT_WARNING(&m_Device, "Stencil is read-only, but the pipeline writes to stencil. Writing happens only in VK!");
    }
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
