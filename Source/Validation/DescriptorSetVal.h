// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorSetVal : public DeviceObjectVal<DescriptorSet> {
    DescriptorSetVal(DeviceVal& device) : DeviceObjectVal(device) {
    }

    inline const DescriptorSetDesc& GetDesc() const {
        return *m_Desc;
    }

    inline void SetImpl(DescriptorSet* impl, const DescriptorSetDesc* desc) {
        m_Impl = impl;
        m_Desc = desc;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================
    void SetDebugName(const char* name);
    void UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs);
    void UpdateDynamicConstantBuffers(uint32_t baseBuffer, uint32_t bufferNum, const Descriptor* const* descriptors);
    void Copy(const DescriptorSetCopyDesc& descriptorSetCopyDesc);

  private:
    const DescriptorSetDesc* m_Desc = nullptr;
};

} // namespace nri
