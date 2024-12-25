#pragma once

#import <MetalKit/MetalKit.h>

namespace nri {

struct DeviceMTL;

struct DescriptorSetLayout {
    DescriptorSetDesc m_DescriptorSetDesc;
    NSMutableArray<MTLArgumentDescriptor*>* m_ArgumentDescriptors;
};

struct PipelineLayoutMTL {

    inline PipelineLayoutMTL (DeviceMTL& device)
        : m_Device(device)
        , m_HasVariableDescriptorNum(device.GetStdAllocator())
        , m_DescriptorSetRangeDescs(device.GetStdAllocator())
        , m_DynamicConstantBufferDescs(device.GetStdAllocator())
        , m_DescriptorSetLayouts(device.GetStdAllocator())
    {
    }

    ~PipelineLayoutMTL();
    
    inline DeviceMTL& GetDevice() const {
        return m_Device;
    }
    
    const inline struct DescriptorSetLayout* GetDescriptorSetLayout(uint32_t setIndex) const {
        return &m_DescriptorSetLayouts[setIndex];
    }

    Result Create(const PipelineLayoutDesc& pipelineLayoutDesc);
   

private:
    DeviceMTL& m_Device;
    
    Vector<bool> m_HasVariableDescriptorNum;
    Vector<DescriptorRangeDesc> m_DescriptorSetRangeDescs;
    Vector<DynamicConstantBufferDesc> m_DynamicConstantBufferDescs;
    Vector<DescriptorSetLayout> m_DescriptorSetLayouts;

};

}
