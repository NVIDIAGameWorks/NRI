// © 2021 NVIDIA Corporation

#pragma once

namespace nri
{

struct DeviceD3D12;

struct DescriptorPoolD3D12
{
    inline DescriptorPoolD3D12(DeviceD3D12& device)
        : m_Device(device)
        , m_DescriptorSets(device.GetStdAllocator())
    {}

    inline ~DescriptorPoolD3D12()
    {
        Reset();
        m_DescriptorSets.clear();
    }

    inline DeviceD3D12& GetDevice() const
    { return m_Device; }

    Result Create(const DescriptorPoolDesc& descriptorPoolDesc);
    void Bind(ID3D12GraphicsCommandList* graphicsCommandList) const;
    uint32_t AllocateDescriptors(DescriptorHeapType descriptorHeapType, uint32_t descriptorNum);
    DescriptorPointerCPU GetDescriptorPointerCPU(DescriptorHeapType descriptorHeapType, uint32_t offset) const;
    DescriptorPointerGPU GetDescriptorPointerGPU(DescriptorHeapType descriptorHeapType, uint32_t offset) const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    Result AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndexInPipelineLayout, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum);
    void Reset();

private:
    DeviceD3D12& m_Device;
    std::array<DescriptorHeapDesc, DescriptorHeapType::MAX_NUM> m_DescriptorHeapDescs;
    std::array<uint32_t, DescriptorHeapType::MAX_NUM> m_DescriptorNum = {};
    std::array<ID3D12DescriptorHeap*, DescriptorHeapType::MAX_NUM> m_DescriptorHeaps = {};
    Vector<DescriptorSetD3D12> m_DescriptorSets;
    uint32_t m_DescriptorHeapNum = 0;
    uint32_t m_DescriptorSetNum = 0;
};

}
