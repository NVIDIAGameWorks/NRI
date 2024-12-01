
#include "SharedMTL.h"

#include "DescriptorSetMTL.h"
#include "DescriptorMTL.h"

using namespace nri;


void DescriptorSetMTL::Create(size_t argumentBufferOffset, id<MTLBuffer> argumentBuffer, NSArray<MTLArgumentDescriptor *>* argDesc, const struct DescriptorSetDesc* desc) {
    m_ArgumentDescriptor = argDesc;
    m_ArgumentBuffer = argumentBuffer;
    m_ArgumentBufferOffset = argumentBufferOffset;
    m_Desc = desc;
    m_ArgumentEncoder = [m_Device newArgumentEncoderWithArguments: argDesc];
    [m_ArgumentEncoder setArgumentBuffer:m_ArgumentBuffer offset:argumentBufferOffset];
}

size_t DescriptorSetMTL::getDescriptorLength() {
    return [m_ArgumentEncoder encodedLength];
}

void DescriptorSetMTL::UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    
    for(size_t j = 0; j < rangeNum; j++) {
        const DescriptorRangeUpdateDesc& update = rangeUpdateDescs[j];
        for(size_t descIdx = 0; descIdx < update.descriptorNum; descIdx++) {
            DescriptorMTL* descriptorImpl = (DescriptorMTL*)&update.descriptors[descIdx];
            const DescriptorRangeDesc& rangeDesc = m_Desc->ranges[rangeOffset + j];
            
            switch(descriptorImpl->GetType()) {
                case DescriptorTypeMTL::IMAGE_VIEW_1D:
                case DescriptorTypeMTL::IMAGE_VIEW_2D:
                case DescriptorTypeMTL::IMAGE_VIEW_3D:
                    [m_ArgumentEncoder setTexture: descriptorImpl->GetTextureHandle() atIndex: rangeDesc.baseRegisterIndex + descIdx];
                    break;
                case DescriptorTypeMTL::SAMPLER:
                    [m_ArgumentEncoder setSamplerState: descriptorImpl->GetSamplerStateHandler() atIndex:rangeDesc.baseRegisterIndex + descIdx]; // not sure if this is correct
                    break;
                case DescriptorTypeMTL::BUFFER_VIEW: {
                    BufferViewDesc* view = &descriptorImpl->BufferView();
                    [m_ArgumentEncoder setBuffer: descriptorImpl->GetBufferHandle() offset: view->offset atIndex: rangeDesc.baseRegisterIndex + descIdx];
                    break;
                }
                default:
                    break;
            }
            
        }
    }
}
