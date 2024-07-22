// © 2021 NVIDIA Corporation

#pragma region[  RayTracing  ]

static void NRI_CALL GetAccelerationStructureMemoryDesc(const AccelerationStructure& accelerationStructure, MemoryDesc& memoryDesc) {
    ((const AccelerationStructureVal&)accelerationStructure).GetMemoryDesc(memoryDesc);
}

static uint64_t NRI_CALL GetAccelerationStructureUpdateScratchBufferSize(const AccelerationStructure& accelerationStructure) {
    return ((const AccelerationStructureVal&)accelerationStructure).GetUpdateScratchBufferSize();
}

static uint64_t NRI_CALL GetAccelerationStructureBuildScratchBufferSize(const AccelerationStructure& accelerationStructure) {
    return ((const AccelerationStructureVal&)accelerationStructure).GetBuildScratchBufferSize();
}

static uint64_t NRI_CALL GetAccelerationStructureHandle(const AccelerationStructure& accelerationStructure) {
    return ((const AccelerationStructureVal&)accelerationStructure).GetHandle();
}

static Result NRI_CALL CreateAccelerationStructureDescriptor(const AccelerationStructure& accelerationStructure, Descriptor*& descriptor) {
    return ((AccelerationStructureVal&)accelerationStructure).CreateDescriptor(descriptor);
}

static void NRI_CALL SetAccelerationStructureDebugName(AccelerationStructure& accelerationStructure, const char* name) {
    ((AccelerationStructureVal&)accelerationStructure).SetDebugName(name);
}

static uint64_t NRI_CALL GetAccelerationStructureNativeObject(const AccelerationStructure& accelerationStructure) {
    return ((AccelerationStructureVal&)accelerationStructure).GetNativeObject();
}

#pragma endregion

Define_RayTracing_AccelerationStructure_PartiallyFillFunctionTable(Val)
