#pragma once

#import <MetalKit/MetalKit.h>

namespace nri {

struct DeviceMTL;


//
//struct BindingInfo {
//    BindingInfo(StdAllocator<uint8_t>& allocator);
////    Vector<PushConstantBindingDesc> pushConstantBindings;
////    Vector<PushDescriptorBindingDesc> pushDescriptorBindings;
//};

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
        , m_DescriptorSets(device.GetStdAllocator())
    {
    }

    ~PipelineLayoutMTL();
    
    inline DeviceMTL& GetDevice() const {
        return m_Device;
    }
    
    inline struct DescriptorSetLayout* GetDescriptorSetLayout(uint32_t setIndex) {
        return &m_DescriptorSets[setIndex];
    }
    
//    inline struct DescriptorSetDesc* GetDescriptorSetDesc(uint32_t setIndex) {
//        return &m_DescriptorSetDesc[setIndex];
//    }
    
    Result Create(const PipelineLayoutDesc& pipelineLayoutDesc);
   

private:
    DeviceMTL& m_Device;
    
    Vector<bool> m_HasVariableDescriptorNum;
    Vector<DescriptorRangeDesc> m_DescriptorSetRangeDescs;
    Vector<DynamicConstantBufferDesc> m_DynamicConstantBufferDescs;
//    Vector<DescriptorSetDesc> m_DescriptorSetDesc;
    Vector<DescriptorSetLayout> m_DescriptorSets;
    
//    BindingInfo m_BindingInfo;
};

}
