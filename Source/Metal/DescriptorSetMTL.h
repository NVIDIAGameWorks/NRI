// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceVK;
struct DescriptorSetMTL {
public:
    inline DescriptorSetMTL (DeviceMTL& device)
    : m_Device(device) {
    }
    
    
    void UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs);
    
    inline id<MTLArgumentEncoder> GetArgumentHandle() {
        return m_ArgumentEncoder;
    }
    
private:
    DeviceMTL& m_Device;
    id<MTLArgumentEncoder>         m_ArgumentEncoder;
    
};


} // namespace nri

