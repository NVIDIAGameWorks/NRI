// © 2021 NVIDIA Corporation

#pragma region [  Core  ]

static void NRI_CALL SetDescriptorPoolDebugName(DescriptorPool& descriptorPool, const char* name)
{
    ((DescriptorPoolVK&)descriptorPool).SetDebugName(name);
}

static Result NRI_CALL AllocateDescriptorSets(DescriptorPool& descriptorPool, const PipelineLayout& pipelineLayout, uint32_t setIndexInPipelineLayout, DescriptorSet** descriptorSets,
    uint32_t instanceNum, uint32_t nodeMask, uint32_t variableDescriptorNum)
{
    return ((DescriptorPoolVK&)descriptorPool).AllocateDescriptorSets(pipelineLayout, setIndexInPipelineLayout, descriptorSets, instanceNum, nodeMask, variableDescriptorNum);
}

static void NRI_CALL ResetDescriptorPool(DescriptorPool& descriptorPool)
{
    ((DescriptorPoolVK&)descriptorPool).Reset();
}

#pragma endregion

Define_Core_DescriptorPool_PartiallyFillFunctionTable(VK)
