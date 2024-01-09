// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct PipelineLayoutVal : public DeviceObjectVal<PipelineLayout> {
    PipelineLayoutVal(DeviceVal& device, PipelineLayout* pipelineLayout, const PipelineLayoutDesc& pipelineLayoutDesc);

    inline const PipelineLayoutDesc& GetPipelineLayoutDesc() const {
        return m_PipelineLayoutDesc;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================
    void SetDebugName(const char* name);

  private:
    PipelineLayoutDesc m_PipelineLayoutDesc;
    Vector<DescriptorSetDesc> m_DescriptorSetDescs;
    Vector<PushConstantDesc> m_PushConstantDescs;
    Vector<DescriptorRangeDesc> m_DescriptorRangeDescs;
    Vector<DynamicConstantBufferDesc> m_DynamicConstantBufferDescs;
};

} // namespace nri
