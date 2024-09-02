#pragma once

#import <MetalKit/MetalKit.h>

namespace nri {

struct DeviceMTL;
struct PipelineLayoutMTL;

struct PipelineMTL {
    inline PipelineMTL(DeviceMTL& device)
        : m_Device(device) {
    }
    ~PipelineMTL();

    Result Create(const GraphicsPipelineDesc& graphicsPipelineDesc);
private:
    DeviceMTL& m_Device;
};

}

