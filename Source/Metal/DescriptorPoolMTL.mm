
#include "SharedMTL.h"
#include "DescriptorPoolMTL.h"
#include "DescriptorSetMTL.h"
#include "PipelineLayoutMTL.h"

using namespace nri;


DescriptorPoolMTL::~DescriptorPoolMTL() {
    
}

Result DescriptorPoolMTL::Create(const DescriptorPoolDesc& descriptorPoolDesc) {
    size_t numArgs = descriptorPoolDesc.samplerMaxNum +
        descriptorPoolDesc.constantBufferMaxNum +
        descriptorPoolDesc.dynamicConstantBufferMaxNum +
        descriptorPoolDesc.textureMaxNum +
        descriptorPoolDesc.storageTextureMaxNum +
        descriptorPoolDesc.bufferMaxNum +
        descriptorPoolDesc.storageBufferMaxNum +
        descriptorPoolDesc.structuredBufferMaxNum +
        descriptorPoolDesc.accelerationStructureMaxNum;
   
    m_ArgumentBuffer = [m_Device
        newBufferWithLength: numArgs * sizeof(uint32_t) options:MTLResourceStorageModeShared];
}


//size_t DescriptorPoolMTL::GetNumberOfArugmentsAlloc() {
//    return [m_ArgumentBuffer length] / sizeof(uint32_t);
//}

//================================================================================================================
// NRI
//================================================================================================================

void DescriptorPoolMTL::SetDebugName(const char* name) {
    
}
void DescriptorPoolMTL::Reset() {
    
}

Result DescriptorPoolMTL::AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    PipelineLayoutMTL* pipelineLayoutMTL = (PipelineLayoutMTL*)&pipelineLayout;
    
    uint32_t freeSetNum = (uint32_t)m_AllocatedSets.size() - m_UsedSets;
    if (freeSetNum < instanceNum) {
        uint32_t newSetNum = instanceNum - freeSetNum;
        uint32_t prevSetNum = (uint32_t)m_AllocatedSets.size();
        m_AllocatedSets.resize(prevSetNum + newSetNum);
        for (size_t i = 0; i < newSetNum; i++) {
            Construct(m_AllocatedSets[prevSetNum + i], 1, m_Device);
        }
    }
    
    const struct DescriptorSetLayout* setLayoutMTL = pipelineLayoutMTL->GetDescriptorSetLayout(setIndex);
    for(uint32_t i = 0; i < instanceNum; i++) {
        descriptorSets[i] = (DescriptorSet*)m_AllocatedSets[m_UsedSets++];
        ((DescriptorSetMTL*)descriptorSets[i])->Create(
                   m_ArgumentOffset,
                   m_ArgumentBuffer,
                   setLayoutMTL->m_ArgumentDescriptors,
                   &setLayoutMTL->m_DescriptorSetDesc);
        m_ArgumentOffset += ((DescriptorSetMTL*)descriptorSets[i])->getDescriptorLength();
    }
    
    
    return Result::SUCCESS;
}

