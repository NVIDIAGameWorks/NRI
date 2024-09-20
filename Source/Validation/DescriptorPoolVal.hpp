// © 2021 NVIDIA Corporation

bool DescriptorPoolVal::CheckDescriptorRange(const DescriptorRangeDesc& rangeDesc, uint32_t variableDescriptorNum) {
    const uint32_t descriptorNum = (rangeDesc.flags & DescriptorRangeBits::VARIABLE_SIZED_ARRAY) ? variableDescriptorNum : rangeDesc.descriptorNum;
    if (descriptorNum > rangeDesc.descriptorNum) {
        REPORT_ERROR(&m_Device, "'variableDescriptorNum=%u' is greater than 'descriptorNum=%u'", variableDescriptorNum, rangeDesc.descriptorNum);
        return false;
    }

    switch (rangeDesc.descriptorType) {
        case DescriptorType::SAMPLER:
            return m_SamplerNum + descriptorNum <= m_Desc.samplerMaxNum;
        case DescriptorType::CONSTANT_BUFFER:
            return m_ConstantBufferNum + descriptorNum <= m_Desc.constantBufferMaxNum;
        case DescriptorType::TEXTURE:
            return m_TextureNum + descriptorNum <= m_Desc.textureMaxNum;
        case DescriptorType::STORAGE_TEXTURE:
            return m_StorageTextureNum + descriptorNum <= m_Desc.storageTextureMaxNum;
        case DescriptorType::BUFFER:
            return m_BufferNum + descriptorNum <= m_Desc.bufferMaxNum;
        case DescriptorType::STORAGE_BUFFER:
            return m_StorageBufferNum + descriptorNum <= m_Desc.storageBufferMaxNum;
        case DescriptorType::STRUCTURED_BUFFER:
            return m_StructuredBufferNum + descriptorNum <= m_Desc.structuredBufferMaxNum;
        case DescriptorType::STORAGE_STRUCTURED_BUFFER:
            return m_StorageStructuredBufferNum + descriptorNum <= m_Desc.storageStructuredBufferMaxNum;
        case DescriptorType::ACCELERATION_STRUCTURE:
            return m_AccelerationStructureNum + descriptorNum <= m_Desc.accelerationStructureMaxNum;
        default:
            REPORT_ERROR(&m_Device, "Unknown descriptor range type: %u", (uint32_t)rangeDesc.descriptorType);
            return false;
    }
}

void DescriptorPoolVal::IncrementDescriptorNum(const DescriptorRangeDesc& rangeDesc, uint32_t variableDescriptorNum) {
    const uint32_t descriptorNum = (rangeDesc.flags & DescriptorRangeBits::VARIABLE_SIZED_ARRAY) ? variableDescriptorNum : rangeDesc.descriptorNum;

    switch (rangeDesc.descriptorType) {
        case DescriptorType::SAMPLER:
            m_SamplerNum += descriptorNum;
            return;
        case DescriptorType::CONSTANT_BUFFER:
            m_ConstantBufferNum += descriptorNum;
            return;
        case DescriptorType::TEXTURE:
            m_TextureNum += descriptorNum;
            return;
        case DescriptorType::STORAGE_TEXTURE:
            m_StorageTextureNum += descriptorNum;
            return;
        case DescriptorType::BUFFER:
            m_BufferNum += descriptorNum;
            return;
        case DescriptorType::STORAGE_BUFFER:
            m_StorageBufferNum += descriptorNum;
            return;
        case DescriptorType::STRUCTURED_BUFFER:
            m_StructuredBufferNum += descriptorNum;
            return;
        case DescriptorType::STORAGE_STRUCTURED_BUFFER:
            m_StorageStructuredBufferNum += descriptorNum;
            return;
        case DescriptorType::ACCELERATION_STRUCTURE:
            m_AccelerationStructureNum += descriptorNum;
            return;
        default:
            REPORT_ERROR(&m_Device, "Unknown descriptor range type: %u", (uint32_t)rangeDesc.descriptorType);
            return;
    }
}

NRI_INLINE void DescriptorPoolVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetDescriptorPoolDebugName(*GetImpl(), name);
}

NRI_INLINE void DescriptorPoolVal::Reset() {
    m_DescriptorSetsNum = 0;
    m_SamplerNum = 0;
    m_ConstantBufferNum = 0;
    m_DynamicConstantBufferNum = 0;
    m_TextureNum = 0;
    m_StorageTextureNum = 0;
    m_BufferNum = 0;
    m_StorageBufferNum = 0;
    m_StructuredBufferNum = 0;
    m_StorageStructuredBufferNum = 0;
    m_AccelerationStructureNum = 0;

    GetCoreInterface().ResetDescriptorPool(*GetImpl());
}

NRI_INLINE Result DescriptorPoolVal::AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    const PipelineLayoutVal& pipelineLayoutVal = (const PipelineLayoutVal&)pipelineLayout;
    const PipelineLayoutDesc& pipelineLayoutDesc = pipelineLayoutVal.GetPipelineLayoutDesc();

    RETURN_ON_FAILURE(&m_Device, instanceNum != 0, Result::INVALID_ARGUMENT, "'instanceNum' is 0");
    RETURN_ON_FAILURE(&m_Device, m_DescriptorSetsNum + instanceNum <= m_Desc.descriptorSetMaxNum, Result::INVALID_ARGUMENT, "the maximum number of descriptor sets exceeded");

    if (!m_SkipValidation) {
        RETURN_ON_FAILURE(&m_Device, setIndex < pipelineLayoutDesc.descriptorSetNum, Result::INVALID_ARGUMENT, "'setIndex' is invalid");

        const DescriptorSetDesc& descriptorSetDesc = pipelineLayoutDesc.descriptorSets[setIndex];
        for (uint32_t i = 0; i < descriptorSetDesc.rangeNum; i++) {
            const DescriptorRangeDesc& rangeDesc = descriptorSetDesc.ranges[i];

            bool enoughDescriptors = CheckDescriptorRange(rangeDesc, variableDescriptorNum);
            RETURN_ON_FAILURE(&m_Device, enoughDescriptors, Result::INVALID_ARGUMENT, "the maximum number of descriptors exceeded ('%s')", GetDescriptorTypeName(rangeDesc.descriptorType));
        }

        bool enoughDescriptors = m_DynamicConstantBufferNum + descriptorSetDesc.dynamicConstantBufferNum <= m_Desc.dynamicConstantBufferMaxNum;
        RETURN_ON_FAILURE(&m_Device, enoughDescriptors, Result::INVALID_ARGUMENT, "the maximum number of descriptors exceeded ('DYNAMIC_CONSTANT_BUFFER')");
    }

    PipelineLayout* pipelineLayoutImpl = NRI_GET_IMPL(PipelineLayout, &pipelineLayout);

    Result result = GetCoreInterface().AllocateDescriptorSets(*GetImpl(), *pipelineLayoutImpl, setIndex, descriptorSets, instanceNum, variableDescriptorNum);
    if (result != Result::SUCCESS)
        return result;

    const DescriptorSetDesc& descriptorSetDesc = pipelineLayoutDesc.descriptorSets[setIndex];

    if (!m_SkipValidation) {
        m_DynamicConstantBufferNum += descriptorSetDesc.dynamicConstantBufferNum;
        for (uint32_t i = 0; i < descriptorSetDesc.rangeNum; i++)
            IncrementDescriptorNum(descriptorSetDesc.ranges[i], variableDescriptorNum);
    }

    for (uint32_t i = 0; i < instanceNum; i++) {
        DescriptorSetVal* descriptorSetVal = &m_DescriptorSets[m_DescriptorSetsNum++];
        descriptorSetVal->SetImpl(descriptorSets[i], &descriptorSetDesc);
        descriptorSets[i] = (DescriptorSet*)descriptorSetVal;
    }

    return result;
}
