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

    inline PipelineMTL(DeviceMTL& device)
        : m_Device(device) {
    }
    ~PipelineMTL();

    Result Create(const GraphicsPipelineDesc& graphicsPipelineDesc);
    Result Create(const ComputePipelineDesc& computePipelineDesc);
    Result Create(const RayTracingPipelineDesc& rayTracingPipelineDesc);

    PipelineType m_pipelineType;
    MTLPrimitiveTopologyClass m_topologyClass;
    MTLPrimitiveType m_primitiveType;
    StageBits m_usedBits;
private:
    DeviceMTL& m_Device;
};

}

