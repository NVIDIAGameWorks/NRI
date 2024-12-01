// © 2021 NVIDIA Corporation

#pragma once
 
#import <MetalKit/MetalKit.h>

namespace nri {

struct DeviceMTL;
struct DescriptorSetMTL;

struct DescriptorPoolMTL {
    inline DescriptorPoolMTL(DeviceMTL& device)
        : m_Device(device)
        , m_AllocatedSets(device.GetStdAllocator()) {
        m_AllocatedSets.reserve(64);
    }

    inline DeviceMTL& GetDevice() const {
        return m_Device;
    }

    ~DescriptorPoolMTL();
    Result Create(const DescriptorPoolDesc& descriptorPoolDesc);

    //================================================================================================================
    // NRI
    //================================================================================================================
    
//    size_t GetNumberOfArugmentsAlloc();
    void SetDebugName(const char* name);
    void Reset();
    Result AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum);

private:
    DeviceMTL& m_Device;
    size_t m_ArgumentOffset = 0;
    Vector<DescriptorSetMTL*> m_AllocatedSets;
    id<MTLBuffer> m_ArgumentBuffer;
    
    uint32_t m_UsedSets = 0;
    bool m_OwnsNativeObjects = true;
};

} // namespace nri
