// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static void NRI_CALL SetDescriptorDebugName(Descriptor& descriptor, const char* name) {
    ((DescriptorVK&)descriptor).SetDebugName(name);
}

static uint64_t NRI_CALL GetDescriptorNativeObject(const Descriptor& descriptor) {
    if (!(&descriptor))
        return 0;

    const DescriptorVK& d = ((DescriptorVK&)descriptor);

    uint64_t handle = 0;
    if (d.GetType() == DescriptorTypeVK::BUFFER_VIEW)
        handle = (uint64_t)d.GetBufferView();
    else if (d.GetType() == DescriptorTypeVK::IMAGE_VIEW)
        handle = (uint64_t)d.GetImageView();
    else if (d.GetType() == DescriptorTypeVK::SAMPLER)
        handle = (uint64_t)d.GetSampler();
    else if (d.GetType() == DescriptorTypeVK::ACCELERATION_STRUCTURE)
        handle = (uint64_t)d.GetAccelerationStructure();

    return handle;
}

#pragma endregion

Define_Core_Descriptor_PartiallyFillFunctionTable(VK);
