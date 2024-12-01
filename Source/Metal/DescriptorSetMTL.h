// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceVK;
struct DescriptorSetMTL {
public:
    inline DescriptorSetMTL (DeviceMTL& device)
    : m_Device(device) {
    }
    
    void Create(size_t argumentBufferOffset, id<MTLBuffer> argumentBuffer, NSArray<MTLArgumentDescriptor *>* argDesc, const struct DescriptorSetDesc* desc);
    void UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs);
    inline id<MTLArgumentEncoder> GetArgumentHandle() {
        return m_ArgumentEncoder;
    }
    size_t getDescriptorLength();
    
private:
    DeviceMTL& m_Device;
    id<MTLArgumentEncoder>    m_ArgumentEncoder;
    id<MTLBuffer> m_ArgumentBuffer;
    size_t m_ArgumentBufferOffset;
    NSArray<MTLArgumentDescriptor *>* m_ArgumentDescriptor;
    const DescriptorSetDesc* m_Desc = nullptr;
};

} // namespace nri

