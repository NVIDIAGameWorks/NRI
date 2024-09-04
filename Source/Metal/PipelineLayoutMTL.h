#pragma once

#import <MetalKit/MetalKit.h>

namespace nri {

struct DeviceMTL;

struct PipelineLayoutMTL {
    inline PipelineLayoutMTL (DeviceMTL& device)
        : m_Device(device) {
    }

    ~PipelineLayoutMTL();
//
    inline Result Create(const PipelineLayoutDesc& pipelineLayoutDesc) {
        return CreateDesc(pipelineLayoutDesc);
    }


    struct PipelineDescriptorSet {
        NSMutableArray<MTLArgumentDescriptor*>* m_ArgumentDescriptors;
    };

private:
    Result CreateDesc(const PipelineLayoutDesc& pipelineLayoutDesc);
    DeviceMTL& m_Device;
    
    std::vector<PipelineDescriptorSet> m_DescriptorSets;
};

}
