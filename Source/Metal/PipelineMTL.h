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
    
    Result Create(const GraphicsPipelineDesc& graphicsPipelineDesc);
    Result Create(const ComputePipelineDesc& computePipelineDesc);
    Result Create(const RayTracingPipelineDesc& rayTracingPipelineDesc);

    inline MTLPrimitiveType GetPrimitiveType() const {
        return m_primitiveType;
    }
    
    inline PipelineType GetPipelineType() const {
        return m_PipelineType;
    }

    inline id<MTLComputePipelineState> GetComputePipeline() const {
        return m_ComputePipeline;
    }
    
    
    inline id<MTLRenderPipelineState> GetGraphicsPipeline() const {
        return m_GraphicsPipeline;
    }
    
    
private:
    PipelineType m_PipelineType;
    MTLPrimitiveTopologyClass m_topologyClass;
    MTLPrimitiveType m_primitiveType;
    StageBits m_usedBits;
    
    union{
        id<MTLComputePipelineState> m_ComputePipeline = nil;
        id<MTLRenderPipelineState> m_GraphicsPipeline;
    };
    DeviceMTL& m_Device;
};

}

