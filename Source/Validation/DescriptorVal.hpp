// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static void NRI_CALL SetDescriptorDebugName(Descriptor& descriptor, const char* name) {
    ((DescriptorVal&)descriptor).SetDebugName(name);
}

static uint64_t NRI_CALL GetDescriptorNativeObject(const Descriptor& descriptor, uint32_t nodeIndex) {
    if (!(&descriptor))
        return 0;

    return ((DescriptorVal&)descriptor).GetNativeObject(nodeIndex);
}

#pragma endregion

Define_Core_Descriptor_PartiallyFillFunctionTable(Val)
