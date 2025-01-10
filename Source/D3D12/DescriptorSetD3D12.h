// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceD3D12;
struct DescriptorPoolD3D12;

struct DescriptorRangeMapping {
    DescriptorHeapType descriptorHeapType;
    uint32_t heapOffset;
    uint32_t descriptorNum;
};

struct DescriptorSetMapping {
    inline DescriptorSetMapping(StdAllocator<uint8_t>& allocator)
        : descriptorRangeMappings(allocator) {
    }

    std::array<uint32_t, DescriptorHeapType::MAX_NUM> descriptorNum = {};
    Vector<DescriptorRangeMapping> descriptorRangeMappings;
};

struct DescriptorSetD3D12 final : public DebugNameBase {
    DescriptorSetD3D12(DescriptorPoolD3D12& desriptorPoolD3D12);

    void Initialize(const DescriptorSetMapping* descriptorSetMapping, uint16_t dynamicConstantBufferNum);

    static void BuildDescriptorSetMapping(const DescriptorSetDesc& descriptorSetDesc, DescriptorSetMapping& descriptorSetMapping);

    DescriptorPointerCPU GetPointerCPU(uint32_t rangeIndex, uint32_t rangeOffset) const;
    DescriptorPointerGPU GetPointerGPU(uint32_t rangeIndex, uint32_t rangeOffset) const;
    DescriptorPointerGPU GetDynamicPointerGPU(uint32_t dynamicConstantBufferIndex) const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    void UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs);
    void UpdateDynamicConstantBuffers(uint32_t baseDynamicConstantBuffer, uint32_t dynamicConstantBufferNum, const Descriptor* const* descriptors);
    void Copy(const DescriptorSetCopyDesc& descriptorSetCopyDesc);

private:
    DescriptorPoolD3D12& m_DescriptorPoolD3D12;
    Vector<DescriptorPointerGPU> m_DynamicConstantBuffers;
    std::array<uint32_t, DescriptorHeapType::MAX_NUM> m_HeapOffset = {};
    const DescriptorSetMapping* m_DescriptorSetMapping = nullptr;
};

} // namespace nri
