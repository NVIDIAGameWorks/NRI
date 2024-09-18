// © 2021 NVIDIA Corporation

PipelineVal::PipelineVal(DeviceVal& device, Pipeline* pipeline)
    : DeviceObjectVal(device, pipeline) {
}

PipelineVal::PipelineVal(DeviceVal& device, Pipeline* pipeline, const GraphicsPipelineDesc& graphicsPipelineDesc)
    : DeviceObjectVal(device, pipeline)
    , m_PipelineLayout(graphicsPipelineDesc.pipelineLayout) {
    m_WritesToDepth = graphicsPipelineDesc.outputMerger.depth.write;
    m_WritesToStencil = graphicsPipelineDesc.outputMerger.stencil.front.writeMask != 0 || graphicsPipelineDesc.outputMerger.stencil.back.writeMask != 0;
}

PipelineVal::PipelineVal(DeviceVal& device, Pipeline* pipeline, const ComputePipelineDesc& computePipelineDesc)
    : DeviceObjectVal(device, pipeline)
    , m_PipelineLayout(computePipelineDesc.pipelineLayout) {
}

PipelineVal::PipelineVal(DeviceVal& device, Pipeline* pipeline, const RayTracingPipelineDesc& rayTracingPipelineDesc)
    : DeviceObjectVal(device, pipeline)
    , m_PipelineLayout(rayTracingPipelineDesc.pipelineLayout) {
}

NRI_INLINE void PipelineVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetPipelineDebugName(*GetImpl(), name);
}

NRI_INLINE Result PipelineVal::WriteShaderGroupIdentifiers(uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* buffer) {
    return GetRayTracingInterface().WriteShaderGroupIdentifiers(*GetImpl(), baseShaderGroupIndex, shaderGroupNum, buffer);
}
