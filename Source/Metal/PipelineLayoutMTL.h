#pragma once

#import <MetalKit/MetalKit.h>

namespace nri {

struct DeviceMTL;

struct PipelineLayoutMTL {
    inline PipelineLayoutMTL (DeviceMTL& device)
        : m_Device(device) {
    }

    ~PipelineLayoutMTL();

    Result Create(const PipelineLayoutDesc& pipelineLayoutDesc);
    
    struct PipelineDescriptorSet {
        NSMutableArray<MTLArgumentDescriptor*>* m_ArgumentDescriptors;
    };

private:
    DeviceMTL& m_Device;
    
    std::vector<PipelineDescriptorSet> m_DescriptorSets;
};

}
