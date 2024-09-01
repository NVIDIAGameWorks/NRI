#pragma once

namespace nri {

struct DeviceMTL;

struct PipelineLayoutMTL {
    inline PipelineLayoutMTL (DeviceMTL& device)
        : m_Device(device) {
    }

//    inline operator VkPipelineLayout() const {
//        return m_Handle;
//    }
//
//    inline DeviceVK& GetDevice() const {
//        return m_Device;
//    }
//
//    inline const RuntimeBindingInfo& GetRuntimeBindingInfo() const {
//        return m_RuntimeBindingInfo;
//    }
//
//    inline VkDescriptorSetLayout GetDescriptorSetLayout(uint32_t index) const {
//        return m_DescriptorSetLayouts[index];
//    }
//
//    inline VkPipelineBindPoint GetPipelineBindPoint() const {
//        return m_PipelineBindPoint;
//    }
//
//    inline uint32_t GetDescriptorSetSpace(uint32_t setIndexInPipelineLayout) const {
//        return m_DescriptorSetSpaces[setIndexInPipelineLayout];
//    }
//
    ~PipelineLayoutMTL();
//
//    Result Create(const PipelineLayoutDesc& pipelineLayoutDesc);
//
//    //================================================================================================================
//    // NRI
//    //================================================================================================================
//
//    void SetDebugName(const char* name);


private:
    DeviceMTL& m_Device;
    id<MTLRenderPipelineDescriptor*> m_PipelineDesc;
    //Vector<MTLArgumentDescriptor*> m_DescriptorSetLayouts;

};

}
