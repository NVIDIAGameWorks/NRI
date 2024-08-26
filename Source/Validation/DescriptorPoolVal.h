// © 2021 NVIDIA Corporation

#pragma once

#include "DescriptorSetVal.h"

namespace nri {

struct DescriptorPoolVal : public DeviceObjectVal<DescriptorPool> {
    DescriptorPoolVal(DeviceVal& device, DescriptorPool* descriptorPool, uint32_t descriptorSetMaxNum)
        : DeviceObjectVal(device, descriptorPool)
        , m_DescriptorSets(device.GetStdAllocator())
        , m_SkipValidation(true) // TODO: we have to request "DescriptorPoolDesc" in "DescriptorPoolVKDesc"
    {
        m_Desc.descriptorSetMaxNum = descriptorSetMaxNum;
        m_DescriptorSets.reserve(m_Desc.descriptorSetMaxNum);
        for (uint32_t i = 0; i < m_Desc.descriptorSetMaxNum; i++)
            m_DescriptorSets.emplace_back(DescriptorSetVal(device));
    }

    DescriptorPoolVal(DeviceVal& device, DescriptorPool* descriptorPool, const DescriptorPoolDesc& descriptorPoolDesc)
        : DeviceObjectVal(device, descriptorPool)
        , m_DescriptorSets(device.GetStdAllocator())
        , m_Desc(descriptorPoolDesc) {
        m_DescriptorSets.reserve(m_Desc.descriptorSetMaxNum);
        for (uint32_t i = 0; i < m_Desc.descriptorSetMaxNum; i++)
            m_DescriptorSets.emplace_back(DescriptorSetVal(device));
    }

    //================================================================================================================
    // NRI
    //================================================================================================================
    void SetDebugName(const char* name);
    void Reset();

    Result AllocateDescriptorSets(
        const PipelineLayout& pipelineLayout, uint32_t setIndexInPipelineLayout, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum);

private:
    bool CheckDescriptorRange(const DescriptorRangeDesc& rangeDesc, uint32_t variableDescriptorNum);
    void IncrementDescriptorNum(const DescriptorRangeDesc& rangeDesc, uint32_t variableDescriptorNum);

    Vector<DescriptorSetVal> m_DescriptorSets;
    DescriptorPoolDesc m_Desc = {};
    uint32_t m_DescriptorSetsNum = 0;
    uint32_t m_SamplerNum = 0;
    uint32_t m_ConstantBufferNum = 0;
    uint32_t m_DynamicConstantBufferNum = 0;
    uint32_t m_TextureNum = 0;
    uint32_t m_StorageTextureNum = 0;
    uint32_t m_BufferNum = 0;
    uint32_t m_StorageBufferNum = 0;
    uint32_t m_StructuredBufferNum = 0;
    uint32_t m_StorageStructuredBufferNum = 0;
    uint32_t m_AccelerationStructureNum = 0;
    bool m_SkipValidation = false;
};

} // namespace nri
