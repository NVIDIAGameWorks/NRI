// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static void NRI_CALL SetDescriptorPoolDebugName(DescriptorPool& descriptorPool, const char* name) {
    ((DescriptorPoolVal&)descriptorPool).SetDebugName(name);
}

static Result NRI_CALL AllocateDescriptorSets(DescriptorPool& descriptorPool, const PipelineLayout& pipelineLayout, uint32_t setIndex,
    DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    return ((DescriptorPoolVal&)descriptorPool).AllocateDescriptorSets(pipelineLayout, setIndex, descriptorSets, instanceNum, variableDescriptorNum);
}

static void NRI_CALL ResetDescriptorPool(DescriptorPool& descriptorPool) {
    ((DescriptorPoolVal&)descriptorPool).Reset();
}

#pragma endregion

Define_Core_DescriptorPool_PartiallyFillFunctionTable(Val);
