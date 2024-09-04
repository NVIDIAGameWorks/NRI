#include "SharedMTL.h"

#include "PipelineLayoutMTL.h"

using namespace nri;

PipelineLayoutMTL::~PipelineLayoutMTL() {

}

Result PipelineLayoutMTL::CreateDesc(const PipelineLayoutDesc& pipelineLayoutDesc) {
    m_DescriptorSets.resize(pipelineLayoutDesc.descriptorSetNum);
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        const DescriptorSetDesc& descriptorSetDesc = pipelineLayoutDesc.descriptorSets[i];
        
        NSMutableArray<MTLArgumentDescriptor*>* argumentDescriptors =  [[NSMutableArray alloc] init];
        MTLArgumentDescriptor* argDescriptor = [MTLArgumentDescriptor argumentDescriptor];
        
        for(size_t r = 0; r < descriptorSetDesc.rangeNum; r++) {

        }


        //argDescriptor.access = memberDescriptor.mAccessType;
        //argDescriptor.arrayLength = memberDescriptor.mArrayLength;
        //argDescriptor.constantBlockAlignment = memberDescriptor.mAlignment;
        //argDescriptor.dataType = memberDescriptor.mDataType;
        //argDescriptor.index = memberDescriptor.mArgumentIndex;
        //argDescriptor.textureType = memberDescriptor.mTextureType;
        
        [argumentDescriptors addObject:argDescriptor];
        m_DescriptorSets[i].m_ArgumentDescriptors = argumentDescriptors;

    }
    return Result::SUCCESS;
}

