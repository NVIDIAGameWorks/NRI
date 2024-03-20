// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct PipelineVal : public DeviceObjectVal<Pipeline> {
    PipelineVal(DeviceVal& device, Pipeline* pipeline);
    PipelineVal(DeviceVal& device, Pipeline* pipeline, const GraphicsPipelineDesc& graphicsPipelineDesc);
    PipelineVal(DeviceVal& device, Pipeline* pipeline, const ComputePipelineDesc& computePipelineDesc);
    PipelineVal(DeviceVal& device, Pipeline* pipeline, const RayTracingPipelineDesc& rayTracingPipelineDesc);

    inline const PipelineLayout* GetPipelineLayout() const {
        return m_PipelineLayout;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================
    void SetDebugName(const char* name);
    Result WriteShaderGroupIdentifiers(uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* buffer);

private:
    const PipelineLayout* m_PipelineLayout = nullptr;
};

} // namespace nri
