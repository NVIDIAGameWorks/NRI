// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static void NRI_CALL SetDescriptorSetDebugName(DescriptorSet& descriptorSet, const char* name) {
    ((DescriptorSetD3D12&)descriptorSet).SetDebugName(name);
}

static void NRI_CALL
UpdateDescriptorRanges(DescriptorSet& descriptorSet, uint32_t nodeMask, uint32_t baseRange, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    MaybeUnused(nodeMask); // TODO: use it

    ((DescriptorSetD3D12&)descriptorSet).UpdateDescriptorRanges(baseRange, rangeNum, rangeUpdateDescs);
}

static void NRI_CALL UpdateDynamicConstantBuffers(DescriptorSet& descriptorSet, uint32_t nodeMask, uint32_t baseBuffer, uint32_t bufferNum, const Descriptor* const* descriptors) {
    MaybeUnused(nodeMask); // TODO: use it

    ((DescriptorSetD3D12&)descriptorSet).UpdateDynamicConstantBuffers(baseBuffer, bufferNum, descriptors);
}

static void NRI_CALL CopyDescriptorSet(DescriptorSet& descriptorSet, const DescriptorSetCopyDesc& descriptorSetCopyDesc) {
    ((DescriptorSetD3D12&)descriptorSet).Copy(descriptorSetCopyDesc);
}

#pragma endregion

Define_Core_DescriptorSet_PartiallyFillFunctionTable(D3D12)
