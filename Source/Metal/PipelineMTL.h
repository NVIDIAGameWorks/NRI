#pragma once

#import <MetalKit/MetalKit.h>

namespace nri {

struct DeviceMTL;
struct PipelineLayoutMTL;

NriEnum(PipelineType, uint8_t,
    Compute,
    Graphics,
    Raytracing
);

struct PipelineMTL {
public:
    inline PipelineMTL(DeviceMTL& device)
        : m_Device(device) {
    }
    ~PipelineMTL();
    
    inline PipelineType GetPipelineType() {
        return m_PipelineType;
    }

    Result Create(const GraphicsPipelineDesc& graphicsPipelineDesc);
    Result Create(const ComputePipelineDesc& computePipelineDesc);
    Result Create(const RayTracingPipelineDesc& rayTracingPipelineDesc);

    MTLPrimitiveTopologyClass m_topologyClass;
    MTLPrimitiveType m_primitiveType;
    StageBits m_usedBits;
    
private:
    PipelineType m_PipelineType;
    union{
        id<MTLComputePipelineState> m_ComputePipeline = nil;
        id<MTLRenderPipelineState> m_GraphicsPipeline;
    };
    DeviceMTL& m_Device;
};

}

