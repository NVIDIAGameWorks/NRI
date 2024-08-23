// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static void NRI_CALL SetCommandBufferDebugName(CommandBuffer& commandBuffer, const char* name) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetDebugName(name);
}

static Result NRI_CALL BeginCommandBuffer(CommandBuffer& commandBuffer, const DescriptorPool* descriptorPool) {
    return ((CommandBufferEmuD3D11&)commandBuffer).Begin(descriptorPool);
}

static Result NRI_CALL EndCommandBuffer(CommandBuffer& commandBuffer) {
    return ((CommandBufferEmuD3D11&)commandBuffer).End();
}

static void NRI_CALL CmdSetPipelineLayout(CommandBuffer& commandBuffer, const PipelineLayout& pipelineLayout) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetPipelineLayout(pipelineLayout);
}

static void NRI_CALL CmdSetPipeline(CommandBuffer& commandBuffer, const Pipeline& pipeline) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetPipeline(pipeline);
}

static void NRI_CALL CmdBarrier(CommandBuffer& commandBuffer, const BarrierGroupDesc& barrierGroupDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).Barrier(barrierGroupDesc);
}

static void NRI_CALL CmdSetDescriptorPool(CommandBuffer& commandBuffer, const DescriptorPool& descriptorPool) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetDescriptorPool(descriptorPool);
}

static void NRI_CALL CmdSetDescriptorSet(
    CommandBuffer& commandBuffer, uint32_t setIndexInPipelineLayout, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetDescriptorSet(setIndexInPipelineLayout, descriptorSet, dynamicConstantBufferOffsets);
}

static void NRI_CALL CmdSetConstants(CommandBuffer& commandBuffer, uint32_t pushConstantIndex, const void* data, uint32_t size) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetConstants(pushConstantIndex, data, size);
}

static void NRI_CALL CmdBeginRendering(CommandBuffer& commandBuffer, const AttachmentsDesc& attachmentsDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).BeginRendering(attachmentsDesc);
}

static void NRI_CALL CmdEndRendering(CommandBuffer& commandBuffer) {
    ((CommandBufferEmuD3D11&)commandBuffer).EndRendering();
}

static void NRI_CALL CmdSetViewports(CommandBuffer& commandBuffer, const Viewport* viewports, uint32_t viewportNum) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetViewports(viewports, viewportNum);
}

static void NRI_CALL CmdSetScissors(CommandBuffer& commandBuffer, const Rect* rects, uint32_t rectNum) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetScissors(rects, rectNum);
}

static void NRI_CALL CmdSetDepthBounds(CommandBuffer& commandBuffer, float boundsMin, float boundsMax) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetDepthBounds(boundsMin, boundsMax);
}

static void NRI_CALL CmdSetStencilReference(CommandBuffer& commandBuffer, uint8_t frontRef, uint8_t backRef) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetStencilReference(frontRef, backRef);
}

static void NRI_CALL CmdSetSamplePositions(CommandBuffer& commandBuffer, const SamplePosition* positions, Sample_t positionNum, Sample_t sampleNum) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetSamplePositions(positions, positionNum, sampleNum);
}

static void NRI_CALL CmdSetBlendConstants(CommandBuffer& commandBuffer, const Color32f& color) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetBlendConstants(color);
}

static void NRI_CALL CmdClearAttachments(CommandBuffer& commandBuffer, const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    ((CommandBufferEmuD3D11&)commandBuffer).ClearAttachments(clearDescs, clearDescNum, rects, rectNum);
}

static void NRI_CALL CmdSetIndexBuffer(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, IndexType indexType) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetIndexBuffer(buffer, offset, indexType);
}

static void NRI_CALL CmdSetVertexBuffers(CommandBuffer& commandBuffer, uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetVertexBuffers(baseSlot, bufferNum, buffers, offsets);
}

static void NRI_CALL CmdDraw(CommandBuffer& commandBuffer, const DrawDesc& drawDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).Draw(drawDesc);
}

static void NRI_CALL CmdDrawIndexed(CommandBuffer& commandBuffer, const DrawIndexedDesc& drawIndexedDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).DrawIndexed(drawIndexedDesc);
}

static void NRI_CALL CmdDrawIndirect(
    CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferEmuD3D11&)commandBuffer).DrawIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

static void NRI_CALL CmdDrawIndexedIndirect(
    CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferEmuD3D11&)commandBuffer).DrawIndexedIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

static void NRI_CALL CmdDispatch(CommandBuffer& commandBuffer, const DispatchDesc& dispatchDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).Dispatch(dispatchDesc);
}

static void NRI_CALL CmdDispatchIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset) {
    ((CommandBufferEmuD3D11&)commandBuffer).DispatchIndirect(buffer, offset);
}

static void NRI_CALL CmdBeginQuery(CommandBuffer& commandBuffer, const QueryPool& queryPool, uint32_t offset) {
    ((CommandBufferEmuD3D11&)commandBuffer).BeginQuery(queryPool, offset);
}

static void NRI_CALL CmdEndQuery(CommandBuffer& commandBuffer, const QueryPool& queryPool, uint32_t offset) {
    ((CommandBufferEmuD3D11&)commandBuffer).EndQuery(queryPool, offset);
}

static void NRI_CALL CmdBeginAnnotation(CommandBuffer& commandBuffer, const char* name) {
    ((CommandBufferEmuD3D11&)commandBuffer).BeginAnnotation(name);
}

static void NRI_CALL CmdEndAnnotation(CommandBuffer& commandBuffer) {
    ((CommandBufferEmuD3D11&)commandBuffer).EndAnnotation();
}

static void NRI_CALL CmdClearStorageBuffer(CommandBuffer& commandBuffer, const ClearStorageBufferDesc& clearDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).ClearStorageBuffer(clearDesc);
}

static void NRI_CALL CmdClearStorageTexture(CommandBuffer& commandBuffer, const ClearStorageTextureDesc& clearDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).ClearStorageTexture(clearDesc);
}

static void NRI_CALL CmdCopyBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    ((CommandBufferEmuD3D11&)commandBuffer).CopyBuffer(dstBuffer, dstOffset, srcBuffer, srcOffset, size);
}

static void NRI_CALL CmdCopyTexture(
    CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).CopyTexture(dstTexture, dstRegionDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdUploadBufferToTexture(
    CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).UploadBufferToTexture(dstTexture, dstRegionDesc, srcBuffer, srcDataLayoutDesc);
}

static void NRI_CALL CmdReadbackTextureToBuffer(
    CommandBuffer& commandBuffer, Buffer& dstBuffer, TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).ReadbackTextureToBuffer(dstBuffer, dstDataLayoutDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdCopyQueries(CommandBuffer& commandBuffer, const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    ((CommandBufferEmuD3D11&)commandBuffer).CopyQueries(queryPool, offset, num, dstBuffer, dstOffset);
}

static void NRI_CALL CmdResetQueries(CommandBuffer&, const QueryPool&, uint32_t, uint32_t) {
}

static void* NRI_CALL GetCommandBufferNativeObject(const CommandBuffer& commandBuffer) {
    if (!(&commandBuffer))
        return nullptr;

    CommandBufferHelper& commandBufferHelper = (CommandBufferHelper&)commandBuffer;
    return commandBufferHelper.GetNativeObject();
}

#pragma endregion

void Core_CommandBufferEmu_PartiallyFillFunctionTable(CoreInterface& table) {
    table.BeginCommandBuffer = ::BeginCommandBuffer;
    table.CmdSetDescriptorPool = ::CmdSetDescriptorPool;
    table.CmdSetDescriptorSet = ::CmdSetDescriptorSet;
    table.CmdSetPipelineLayout = ::CmdSetPipelineLayout;
    table.CmdSetPipeline = ::CmdSetPipeline;
    table.CmdSetConstants = ::CmdSetConstants;
    table.CmdBarrier = ::CmdBarrier;
    table.CmdBeginRendering = ::CmdBeginRendering;
    table.CmdClearAttachments = ::CmdClearAttachments;
    table.CmdSetViewports = ::CmdSetViewports;
    table.CmdSetScissors = ::CmdSetScissors;
    table.CmdSetDepthBounds = ::CmdSetDepthBounds;
    table.CmdSetStencilReference = ::CmdSetStencilReference;
    table.CmdSetSamplePositions = ::CmdSetSamplePositions;
    table.CmdSetBlendConstants = ::CmdSetBlendConstants;
    table.CmdSetIndexBuffer = ::CmdSetIndexBuffer;
    table.CmdSetVertexBuffers = ::CmdSetVertexBuffers;
    table.CmdDraw = ::CmdDraw;
    table.CmdDrawIndexed = ::CmdDrawIndexed;
    table.CmdDrawIndirect = ::CmdDrawIndirect;
    table.CmdDrawIndexedIndirect = ::CmdDrawIndexedIndirect;
    table.CmdEndRendering = ::CmdEndRendering;
    table.CmdDispatch = ::CmdDispatch;
    table.CmdDispatchIndirect = ::CmdDispatchIndirect;
    table.CmdBeginQuery = ::CmdBeginQuery;
    table.CmdEndQuery = ::CmdEndQuery;
    table.CmdCopyQueries = ::CmdCopyQueries;
    table.CmdResetQueries = ::CmdResetQueries;
    table.CmdBeginAnnotation = ::CmdBeginAnnotation;
    table.CmdEndAnnotation = ::CmdEndAnnotation;
    table.CmdClearStorageBuffer = ::CmdClearStorageBuffer;
    table.CmdClearStorageTexture = ::CmdClearStorageTexture;
    table.CmdCopyBuffer = ::CmdCopyBuffer;
    table.CmdCopyTexture = ::CmdCopyTexture;
    table.CmdUploadBufferToTexture = ::CmdUploadBufferToTexture;
    table.CmdReadbackTextureToBuffer = ::CmdReadbackTextureToBuffer;
    table.EndCommandBuffer = ::EndCommandBuffer;
    table.SetCommandBufferDebugName = ::SetCommandBufferDebugName;
    table.GetCommandBufferNativeObject = ::GetCommandBufferNativeObject;
}
