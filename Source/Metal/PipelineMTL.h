#pragma once

#import <MetalKit/MetalKit.h>

namespace nri {

struct DeviceMTL;
struct PipelineLayoutMTL;

struct PipelineMTL {
    enum PiplineType {
        None,
        Graphics,
        Compute,
        Raytracing
    };

    inline PipelineMTL(DeviceMTL& device)
        : m_Device(device) {
    }
    ~PipelineMTL();

    Result Create(const GraphicsPipelineDesc& graphicsPipelineDesc);
    Result Create(const ComputePipelineDesc& computePipelineDesc);
    Result Create(const RayTracingPipelineDesc& rayTracingPipelineDesc);
    
    MTLPrimitiveTopologyClass m_topologyClass;
    MTLPrimitiveType m_primitiveType;
private:
    DeviceMTL& m_Device;
};

}

