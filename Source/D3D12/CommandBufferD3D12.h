// © 2021 NVIDIA Corporation

#pragma once

struct ID3D12CommandAllocator;
struct ID3D12Resource;

#ifdef NRI_USE_AGILITY_SDK
struct ID3D12GraphicsCommandList9;
typedef ID3D12GraphicsCommandList9 ID3D12GraphicsCommandListBest;
#else
struct ID3D12GraphicsCommandList6;
typedef ID3D12GraphicsCommandList6 ID3D12GraphicsCommandListBest;
#endif

namespace nri {

struct DeviceD3D12;
struct PipelineD3D12;
struct PipelineLayoutD3D12;
struct DescriptorPoolD3D12;
struct DescriptorSetD3D12;

struct CommandBufferD3D12 {
    inline CommandBufferD3D12(DeviceD3D12& device) : m_Device(device) {
    }

    inline ~CommandBufferD3D12() {
    }

    inline operator ID3D12GraphicsCommandList*() const {
        return m_GraphicsCommandList.GetInterface();
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    Result Create(D3D12_COMMAND_LIST_TYPE commandListType, ID3D12CommandAllocator* commandAllocator);
    Result Create(const CommandBufferD3D12Desc& commandBufferDesc);

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name) {
        SET_D3D_DEBUG_OBJECT_NAME(m_GraphicsCommandList, name);
    }

    Result Begin(const DescriptorPool* descriptorPool);
    Result End();
    void SetViewports(const Viewport* viewports, uint32_t viewportNum);
    void SetScissors(const Rect* rects, uint32_t rectNum);
    void SetDepthBounds(float boundsMin, float boundsMax);
    void SetStencilReference(uint8_t reference);
    void SetSamplePositions(const SamplePosition* positions, uint32_t positionNum);
    void ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum);
    void ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc);
    void ClearStorageTexture(const ClearStorageTextureDesc& clearDesc);
    void BeginRendering(const AttachmentsDesc& attachmentsDesc);
    inline void EndRendering() {
    }
    void SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets);
    void SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType);
    void SetPipelineLayout(const PipelineLayout& pipelineLayout);
    void SetPipeline(const Pipeline& pipeline);
    void SetDescriptorPool(const DescriptorPool& descriptorPool);
    void SetDescriptorSet(uint32_t setIndexInPipelineLayout, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets);
    void SetConstants(uint32_t pushConstantRangeIndex, const void* data, uint32_t size);
    void Draw(uint32_t vertexNum, uint32_t instanceNum, uint32_t baseVertex, uint32_t baseInstance);
    void DrawIndexed(uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance);
    void DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride);
    void DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride);
    void CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size);
    void CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc);
    void UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc);
    void ReadbackTextureToBuffer(Buffer& dstBuffer, TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc);
    void Dispatch(uint32_t x, uint32_t y, uint32_t z);
    void DispatchIndirect(const Buffer& buffer, uint64_t offset);
    void Barrier(const BarrierGroupDesc& barrierGroupDesc);
    void BeginQuery(const QueryPool& queryPool, uint32_t offset);
    void EndQuery(const QueryPool& queryPool, uint32_t offset);
    void CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& buffer, uint64_t alignedBufferOffset);
    void BeginAnnotation(const char* name);
    void EndAnnotation();

    void BuildTopLevelAccelerationStructure(
        uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset
    );
    void BuildBottomLevelAccelerationStructure(
        uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset
    );
    void UpdateTopLevelAccelerationStructure(
        uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, AccelerationStructure& src,
        Buffer& scratch, uint64_t scratchOffset
    );
    void UpdateBottomLevelAccelerationStructure(
        uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, AccelerationStructure& src,
        Buffer& scratch, uint64_t scratchOffset
    );
    void CopyAccelerationStructure(AccelerationStructure& dst, AccelerationStructure& src, CopyMode copyMode);
    void WriteAccelerationStructureSize(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryOffset);
    void DispatchRays(const DispatchRaysDesc& dispatchRaysDesc);

    void DispatchMeshTasks(uint32_t x, uint32_t y, uint32_t z);

  private:
    DeviceD3D12& m_Device;
    ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
    ComPtr<ID3D12GraphicsCommandListBest> m_GraphicsCommandList;
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT> m_RenderTargets = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_DepthStencil = {};
    const PipelineLayoutD3D12* m_PipelineLayout = nullptr;
    PipelineD3D12* m_Pipeline = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    std::array<DescriptorSetD3D12*, 64> m_DescriptorSets = {}; // TODO: 64?
    uint32_t m_RenderTargetNum = 0;
    uint8_t m_Version = 0;
    bool m_IsGraphicsPipelineLayout = false;
};

} // namespace nri
