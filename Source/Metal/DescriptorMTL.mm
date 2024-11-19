#include "SharedMTL.h"

#include "DescriptorMTL.h"

#include "BufferMTL.h"

namespace nri {

Result DescriptorMTL::Create(const BufferViewDesc& bufferViewDesc) {
    //  m_Type = DescriptorTypeVK::BUFFER_VIEW;
    const BufferMTL& buffer = *(const BufferMTL*)bufferViewDesc.buffer;
    
    return Result::SUCCESS;
    
}
Result DescriptorMTL::Create(const Texture1DViewDesc& textureViewDesc) {
    return Result::SUCCESS;
    
}
Result DescriptorMTL::Create(const Texture2DViewDesc& textureViewDesc) {
    
    return Result::SUCCESS;
}
Result DescriptorMTL::Create(const Texture3DViewDesc& textureViewDesc){
    return Result::SUCCESS;
}
Result DescriptorMTL::Create(const SamplerDesc& samplerDesc){
    return Result::SUCCESS;
}

}
