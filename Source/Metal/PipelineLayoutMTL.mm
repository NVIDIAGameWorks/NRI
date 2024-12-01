#include "SharedMTL.h"

#include "PipelineLayoutMTL.h"

using namespace nri;

//
//BindingInfo::BindingInfo(StdAllocator<uint8_t>& allocator)
//    : hasVariableDescriptorNum(allocator)
//    , descriptorSetRangeDescs(allocator)
//    , dynamicConstantBufferDescs(allocator)
//    , descriptorSetDescs(allocator) {
//
//}
//

PipelineLayoutMTL::~PipelineLayoutMTL() {

}

Result PipelineLayoutMTL::Create(const PipelineLayoutDesc& pipelineLayoutDesc) {
   
    
    size_t rangeNum = 0;
    size_t dynamicConstantBufferNum = 0;
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        rangeNum += pipelineLayoutDesc.descriptorSets[i].rangeNum;
        dynamicConstantBufferNum += pipelineLayoutDesc.descriptorSets[i].dynamicConstantBufferNum;
    }
    
    m_DescriptorSets.resize(pipelineLayoutDesc.descriptorSetNum);
    m_HasVariableDescriptorNum.resize(pipelineLayoutDesc.descriptorSetNum);
    m_DescriptorSetRangeDescs.reserve(rangeNum);
    m_DynamicConstantBufferDescs.reserve(dynamicConstantBufferNum);
    
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        const DescriptorSetDesc& descriptorSetDesc = pipelineLayoutDesc.descriptorSets[i];
        
        // Binding info
        m_HasVariableDescriptorNum[i] = false;
        m_DescriptorSets[i].m_DescriptorSetDesc = descriptorSetDesc;
        m_DescriptorSets[i].m_DescriptorSetDesc.ranges = m_DescriptorSetRangeDescs.data() +m_DescriptorSetRangeDescs.size();
        m_DescriptorSets[i].m_DescriptorSetDesc.dynamicConstantBuffers = m_DynamicConstantBufferDescs.data() + m_DynamicConstantBufferDescs.size();
        m_DescriptorSetRangeDescs.insert(m_DescriptorSetRangeDescs.end(), descriptorSetDesc.ranges, descriptorSetDesc.ranges + descriptorSetDesc.rangeNum);
        m_DynamicConstantBufferDescs.insert(m_DynamicConstantBufferDescs.end(), descriptorSetDesc.dynamicConstantBuffers, descriptorSetDesc.dynamicConstantBuffers + descriptorSetDesc.dynamicConstantBufferNum);
        
        NSMutableArray<MTLArgumentDescriptor*>* argumentDescriptors =  [[NSMutableArray alloc] init];
        for(size_t r = 0; r < descriptorSetDesc.rangeNum; r++) {
            MTLArgumentDescriptor* argDescriptor = [MTLArgumentDescriptor argumentDescriptor];
            const DescriptorRangeDesc* range = &descriptorSetDesc.ranges[r];
            argDescriptor.arrayLength = range->descriptorNum;
            argDescriptor.access = MTLBindingAccessReadWrite;
            argDescriptor.index = range->baseRegisterIndex;
            switch(range->descriptorType) {
                case DescriptorType::TEXTURE:
                    argDescriptor.dataType = MTLDataTypeTexture;
                    argDescriptor.textureType = MTLTextureType2D; // descriptor type does not have this
                    break;
                case DescriptorType::SAMPLER:
                    argDescriptor.dataType = MTLDataTypeSampler;
                    break;
                case DescriptorType::CONSTANT_BUFFER:
                case DescriptorType::STORAGE_TEXTURE:
                case DescriptorType::BUFFER:
                case DescriptorType::STORAGE_BUFFER:
                case DescriptorType::STRUCTURED_BUFFER:
                case DescriptorType::STORAGE_STRUCTURED_BUFFER:
                    argDescriptor.dataType = MTLDataTypeStruct;
                    break;
                case DescriptorType::ACCELERATION_STRUCTURE:
                    argDescriptor.dataType = MTLDataTypePrimitiveAccelerationStructure;
                    break;
                default:
                    break;
            }
        }
        m_DescriptorSets[i].m_ArgumentDescriptors = argumentDescriptors;

    }
    return Result::SUCCESS;
}

