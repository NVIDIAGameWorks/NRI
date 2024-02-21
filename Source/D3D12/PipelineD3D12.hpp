// © 2021 NVIDIA Corporation

#pragma region[  RayTracing  ]

static Result NRI_CALL WriteShaderGroupIdentifiers(const Pipeline& pipeline, uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* buffer) {
    return ((const PipelineD3D12&)pipeline).WriteShaderGroupIdentifiers(baseShaderGroupIndex, shaderGroupNum, buffer);
}

void FillFunctionTablePipelineD3D12(RayTracingInterface& rayTracingInterface) {
    rayTracingInterface.WriteShaderGroupIdentifiers = ::WriteShaderGroupIdentifiers;
}

#pragma endregion
