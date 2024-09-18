// © 2021 NVIDIA Corporation

NRI_INLINE void DescriptorSetVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetDescriptorSetDebugName(*GetImpl(), name);
}

NRI_INLINE void DescriptorSetVal::UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    if (rangeNum == 0)
        return;

    RETURN_ON_FAILURE(&m_Device, rangeUpdateDescs != nullptr, ReturnVoid(), "'rangeUpdateDescs' is NULL");
    RETURN_ON_FAILURE(&m_Device, rangeOffset < GetDesc().rangeNum, ReturnVoid(), "'rangeOffset' is out of bounds. (rangeOffset=%u, rangeNum=%u)", rangeOffset, GetDesc().rangeNum);

    RETURN_ON_FAILURE(&m_Device, rangeOffset + rangeNum <= GetDesc().rangeNum, ReturnVoid(),
        "'rangeOffset' + 'rangeNum' is greater than the number of ranges. (rangeOffset=%u, rangeNum=%u, rangeNum=%u)", rangeOffset, rangeNum,
        GetDesc().rangeNum);

    DescriptorRangeUpdateDesc* rangeUpdateDescsImpl = StackAlloc(DescriptorRangeUpdateDesc, rangeNum);
    for (uint32_t i = 0; i < rangeNum; i++) {
        const DescriptorRangeUpdateDesc& updateDesc = rangeUpdateDescs[i];
        const DescriptorRangeDesc& rangeDesc = GetDesc().ranges[rangeOffset + i];

        RETURN_ON_FAILURE(&m_Device, updateDesc.descriptorNum != 0, ReturnVoid(), "'rangeUpdateDescs[%u].descriptorNum' is 0", i);
        RETURN_ON_FAILURE(&m_Device, updateDesc.descriptors != nullptr, ReturnVoid(), "'rangeUpdateDescs[%u].descriptors' is NULL", i);

        RETURN_ON_FAILURE(&m_Device, updateDesc.baseDescriptor < rangeDesc.descriptorNum, ReturnVoid(),
            "'rangeUpdateDescs[%u].baseDescriptor' is greater than the number of descriptors (offsetInRange=%u, rangeDescriptorNum=%u, descriptorType=%s)",
            i, updateDesc.baseDescriptor, rangeDesc.descriptorNum, GetDescriptorTypeName(rangeDesc.descriptorType));

        RETURN_ON_FAILURE(&m_Device, updateDesc.baseDescriptor + updateDesc.descriptorNum <= rangeDesc.descriptorNum, ReturnVoid(),
            "'rangeUpdateDescs[%u].baseDescriptor' + 'rangeUpdateDescs[%u].descriptorNum' is greater than the number of descriptors (offsetInRange=%u, "
            "descriptorNum=%u, rangeDescriptorNum=%u, descriptorType=%s)",
            i, i, updateDesc.baseDescriptor, updateDesc.descriptorNum, rangeDesc.descriptorNum, GetDescriptorTypeName(rangeDesc.descriptorType));

        DescriptorRangeUpdateDesc& dstDesc = rangeUpdateDescsImpl[i];

        dstDesc = updateDesc;
        dstDesc.descriptors = StackAlloc(Descriptor*, updateDesc.descriptorNum);
        Descriptor** descriptors = (Descriptor**)dstDesc.descriptors;

        for (uint32_t j = 0; j < updateDesc.descriptorNum; j++) {
            RETURN_ON_FAILURE(&m_Device, updateDesc.descriptors[j] != nullptr, ReturnVoid(), "'rangeUpdateDescs[%u].descriptors[%u]' is NULL", i, j);

            descriptors[j] = NRI_GET_IMPL(Descriptor, updateDesc.descriptors[j]);
        }
    }

    GetCoreInterface().UpdateDescriptorRanges(*GetImpl(), rangeOffset, rangeNum, rangeUpdateDescsImpl);
}

NRI_INLINE void DescriptorSetVal::UpdateDynamicConstantBuffers(uint32_t baseDynamicConstantBuffer, uint32_t dynamicConstantBufferNum, const Descriptor* const* descriptors) {
    if (dynamicConstantBufferNum == 0)
        return;

    RETURN_ON_FAILURE(&m_Device, baseDynamicConstantBuffer < GetDesc().dynamicConstantBufferNum, ReturnVoid(),
        "'baseDynamicConstantBuffer' is invalid. (baseDynamicConstantBuffer=%u, dynamicConstantBufferNum=%u)", baseDynamicConstantBuffer, GetDesc().dynamicConstantBufferNum);

    RETURN_ON_FAILURE(&m_Device, baseDynamicConstantBuffer + dynamicConstantBufferNum <= GetDesc().dynamicConstantBufferNum, ReturnVoid(),
        "'baseDynamicConstantBuffer' + 'dynamicConstantBufferNum' is greater than the number of buffers (baseDynamicConstantBuffer=%u, dynamicConstantBufferNum=%u, dynamicConstantBufferNum=%u)", baseDynamicConstantBuffer,
        dynamicConstantBufferNum, GetDesc().dynamicConstantBufferNum);

    RETURN_ON_FAILURE(&m_Device, descriptors != nullptr, ReturnVoid(), "'descriptors' is NULL");

    Descriptor** descriptorsImpl = StackAlloc(Descriptor*, dynamicConstantBufferNum);
    for (uint32_t i = 0; i < dynamicConstantBufferNum; i++) {
        RETURN_ON_FAILURE(&m_Device, descriptors[i] != nullptr, ReturnVoid(), "'descriptors[%u]' is NULL", i);

        descriptorsImpl[i] = NRI_GET_IMPL(Descriptor, descriptors[i]);
    }

    GetCoreInterface().UpdateDynamicConstantBuffers(*GetImpl(), baseDynamicConstantBuffer, dynamicConstantBufferNum, descriptorsImpl);
}

NRI_INLINE void DescriptorSetVal::Copy(const DescriptorSetCopyDesc& descriptorSetCopyDesc) {
    RETURN_ON_FAILURE(&m_Device, descriptorSetCopyDesc.srcDescriptorSet != nullptr, ReturnVoid(), "'descriptorSetCopyDesc.srcDescriptorSet' is NULL");

    DescriptorSetVal& srcDescriptorSetVal = *(DescriptorSetVal*)descriptorSetCopyDesc.srcDescriptorSet;
    const DescriptorSetDesc& srcDesc = srcDescriptorSetVal.GetDesc();

    RETURN_ON_FAILURE(&m_Device, descriptorSetCopyDesc.srcBaseRange < srcDesc.rangeNum, ReturnVoid(), "'descriptorSetCopyDesc.srcBaseRange' is invalid");

    bool srcRangeValid = descriptorSetCopyDesc.srcBaseRange + descriptorSetCopyDesc.rangeNum < srcDesc.rangeNum;
    bool dstRangeValid = descriptorSetCopyDesc.dstBaseRange + descriptorSetCopyDesc.rangeNum < GetDesc().rangeNum;
    bool srcOffsetValid = descriptorSetCopyDesc.srcBaseDynamicConstantBuffer < srcDesc.dynamicConstantBufferNum;
    bool srcDynamicConstantBufferValid = descriptorSetCopyDesc.srcBaseDynamicConstantBuffer + descriptorSetCopyDesc.dynamicConstantBufferNum < srcDesc.dynamicConstantBufferNum;
    bool dstOffsetValid = descriptorSetCopyDesc.dstBaseDynamicConstantBuffer < GetDesc().dynamicConstantBufferNum;
    bool dstDynamicConstantBufferValid = descriptorSetCopyDesc.dstBaseDynamicConstantBuffer + descriptorSetCopyDesc.dynamicConstantBufferNum < GetDesc().dynamicConstantBufferNum;

    RETURN_ON_FAILURE(&m_Device, srcRangeValid, ReturnVoid(), "'descriptorSetCopyDesc.rangeNum' is invalid");
    RETURN_ON_FAILURE(&m_Device, descriptorSetCopyDesc.dstBaseRange < GetDesc().rangeNum, ReturnVoid(), "'descriptorSetCopyDesc.dstBaseRange' is invalid");
    RETURN_ON_FAILURE(&m_Device, dstRangeValid, ReturnVoid(), "'descriptorSetCopyDesc.rangeNum' is invalid");
    RETURN_ON_FAILURE(&m_Device, srcOffsetValid, ReturnVoid(), "'descriptorSetCopyDesc.srcBaseDynamicConstantBuffer' is invalid");
    RETURN_ON_FAILURE(&m_Device, srcDynamicConstantBufferValid, ReturnVoid(), "source range of dynamic constant buffers is invalid");
    RETURN_ON_FAILURE(&m_Device, dstOffsetValid, ReturnVoid(), "'descriptorSetCopyDesc.dstBaseDynamicConstantBuffer' is invalid");
    RETURN_ON_FAILURE(&m_Device, dstDynamicConstantBufferValid, ReturnVoid(), "destination range of dynamic constant buffers is invalid");

    auto descriptorSetCopyDescImpl = descriptorSetCopyDesc;
    descriptorSetCopyDescImpl.srcDescriptorSet = NRI_GET_IMPL(DescriptorSet, descriptorSetCopyDesc.srcDescriptorSet);

    GetCoreInterface().CopyDescriptorSet(*GetImpl(), descriptorSetCopyDescImpl);
}
