// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static void NRI_CALL SetDescriptorSetDebugName(DescriptorSet& descriptorSet, const char* name) {
    ((DescriptorSetVal&)descriptorSet).SetDebugName(name);
}

static void NRI_CALL UpdateDescriptorRanges(DescriptorSet& descriptorSet, uint32_t baseRange, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    ((DescriptorSetVal&)descriptorSet).UpdateDescriptorRanges(baseRange, rangeNum, rangeUpdateDescs);
}

static void NRI_CALL UpdateDynamicConstantBuffers(DescriptorSet& descriptorSet, uint32_t baseBuffer, uint32_t bufferNum, const Descriptor* const* descriptors) {
    ((DescriptorSetVal&)descriptorSet).UpdateDynamicConstantBuffers(baseBuffer, bufferNum, descriptors);
}

static void NRI_CALL CopyDescriptorSet(DescriptorSet& descriptorSet, const DescriptorSetCopyDesc& descriptorSetCopyDesc) {
    ((DescriptorSetVal&)descriptorSet).Copy(descriptorSetCopyDesc);
}

#pragma endregion

Define_Core_DescriptorSet_PartiallyFillFunctionTable(Val);
