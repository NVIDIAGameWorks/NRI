// © 2021 NVIDIA Corporation

PipelineLayoutVal::PipelineLayoutVal(DeviceVal& device, PipelineLayout* pipelineLayout, const PipelineLayoutDesc& pipelineLayoutDesc)
    : DeviceObjectVal(device, pipelineLayout)
    , m_DescriptorSetDescs(device.GetStdAllocator())
    , m_RootConstantDescs(device.GetStdAllocator())
    , m_DescriptorRangeDescs(device.GetStdAllocator())
    , m_DynamicConstantBufferDescs(device.GetStdAllocator()) {
    uint32_t descriptorRangeDescNum = 0;
    uint32_t dynamicConstantBufferDescNum = 0;

    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        descriptorRangeDescNum += pipelineLayoutDesc.descriptorSets[i].rangeNum;
        dynamicConstantBufferDescNum += pipelineLayoutDesc.descriptorSets[i].dynamicConstantBufferNum;
    }

    m_DescriptorSetDescs.insert(m_DescriptorSetDescs.begin(), pipelineLayoutDesc.descriptorSets, pipelineLayoutDesc.descriptorSets + pipelineLayoutDesc.descriptorSetNum);
    m_RootConstantDescs.insert(m_RootConstantDescs.begin(), pipelineLayoutDesc.rootConstants, pipelineLayoutDesc.rootConstants + pipelineLayoutDesc.rootConstantNum);

    m_DescriptorRangeDescs.reserve(descriptorRangeDescNum);
    m_DynamicConstantBufferDescs.reserve(dynamicConstantBufferDescNum);

    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        const DescriptorSetDesc& descriptorSetDesc = pipelineLayoutDesc.descriptorSets[i];

        m_DescriptorSetDescs[i].ranges = m_DescriptorRangeDescs.data() + m_DescriptorRangeDescs.size();
        m_DescriptorSetDescs[i].dynamicConstantBuffers = m_DynamicConstantBufferDescs.data() + m_DynamicConstantBufferDescs.size();

        m_DescriptorRangeDescs.insert(m_DescriptorRangeDescs.end(), descriptorSetDesc.ranges, descriptorSetDesc.ranges + descriptorSetDesc.rangeNum);
        m_DynamicConstantBufferDescs.insert(m_DynamicConstantBufferDescs.end(), descriptorSetDesc.dynamicConstantBuffers, descriptorSetDesc.dynamicConstantBuffers + descriptorSetDesc.dynamicConstantBufferNum);
    }

    m_PipelineLayoutDesc = pipelineLayoutDesc;
    m_PipelineLayoutDesc.descriptorSets = m_DescriptorSetDescs.data();
    m_PipelineLayoutDesc.rootConstants = m_RootConstantDescs.data();
}

NRI_INLINE void PipelineLayoutVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetPipelineLayoutDebugName(*GetImpl(), name);
}
