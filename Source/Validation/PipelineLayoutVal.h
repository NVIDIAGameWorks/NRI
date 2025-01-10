// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct PipelineLayoutVal final : public DeviceObjectVal<PipelineLayout> {
    PipelineLayoutVal(DeviceVal& device, PipelineLayout* pipelineLayout, const PipelineLayoutDesc& pipelineLayoutDesc);

    inline const PipelineLayoutDesc& GetPipelineLayoutDesc() const {
        return m_PipelineLayoutDesc;
    }

private:
    PipelineLayoutDesc m_PipelineLayoutDesc;
    Vector<DescriptorSetDesc> m_DescriptorSetDescs;
    Vector<RootConstantDesc> m_RootConstantDescs;
    Vector<DescriptorRangeDesc> m_DescriptorRangeDescs;
    Vector<DynamicConstantBufferDesc> m_DynamicConstantBufferDescs;
};

} // namespace nri
