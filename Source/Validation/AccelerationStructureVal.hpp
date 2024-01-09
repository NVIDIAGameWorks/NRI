// © 2021 NVIDIA Corporation

#pragma region[  RayTracing  ]

static void NRI_CALL GetAccelerationStructureMemoryInfo(const AccelerationStructure& accelerationStructure, MemoryDesc& memoryDesc) {
    ((const AccelerationStructureVal&)accelerationStructure).GetMemoryInfo(memoryDesc);
}

static uint64_t NRI_CALL GetAccelerationStructureUpdateScratchBufferSize(const AccelerationStructure& accelerationStructure) {
    return ((const AccelerationStructureVal&)accelerationStructure).GetUpdateScratchBufferSize();
}

static uint64_t NRI_CALL GetAccelerationStructureBuildScratchBufferSize(const AccelerationStructure& accelerationStructure) {
    return ((const AccelerationStructureVal&)accelerationStructure).GetBuildScratchBufferSize();
}

static uint64_t NRI_CALL GetAccelerationStructureHandle(const AccelerationStructure& accelerationStructure, uint32_t nodeIndex) {
    return ((const AccelerationStructureVal&)accelerationStructure).GetHandle(nodeIndex);
}

static Result NRI_CALL CreateAccelerationStructureDescriptor(const AccelerationStructure& accelerationStructure, uint32_t nodeIndex, Descriptor*& descriptor) {
    return ((AccelerationStructureVal&)accelerationStructure).CreateDescriptor(nodeIndex, descriptor);
}

static void NRI_CALL SetAccelerationStructureDebugName(AccelerationStructure& accelerationStructure, const char* name) {
    ((AccelerationStructureVal&)accelerationStructure).SetDebugName(name);
}

static uint64_t NRI_CALL GetAccelerationStructureNativeObject(const AccelerationStructure& accelerationStructure, uint32_t nodeIndex) {
    return ((AccelerationStructureVal&)accelerationStructure).GetNativeObject(nodeIndex);
}

#pragma endregion

Define_RayTracing_AccelerationStructure_PartiallyFillFunctionTable(Val)
