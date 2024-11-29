#include "SharedMTL.h"

#include "DescriptorMTL.h"

#include "BufferMTL.h"
#include "TextureMTL.h"

#include "ConversionMTL.h"

namespace nri {

DescriptorMTL::~DescriptorMTL() {
    
}


Result DescriptorMTL::Create(const BufferViewDesc& bufferViewDesc) {
    m_Type = DescriptorTypeMTL::BUFFER_VIEW;
    const BufferMTL& buffer = *(const BufferMTL*)bufferViewDesc.buffer;
    
    return Result::SUCCESS;
    
}


Result DescriptorMTL::Create(const Texture1DViewDesc& textureViewDesc) {
    TextureMTL& texture = *(TextureMTL*)textureViewDesc.texture;
    const Mip_t remainingMips = textureViewDesc.mipNum - textureViewDesc.mipOffset;
    const Dim_t remainingLayers = textureViewDesc.layerNum - textureViewDesc.layerOffset;
    
    NSRange level;
    level.location = textureViewDesc.mipOffset;
    level.length = textureViewDesc.mipNum == REMAINING_MIPS ? remainingMips:  textureViewDesc.mipNum;
    NSRange slices;
    slices.location = textureViewDesc.layerNum;
    slices.length = textureViewDesc.layerNum == REMAINING_LAYERS ? remainingLayers : textureViewDesc.layerNum;
    m_Texture = [texture.GetHandle()
                 newTextureViewWithPixelFormat: GetFormatMTL(textureViewDesc.format)
                 textureType: MTLTextureType1D
                 levels:level
                 slices:slices];
    
    return Result::SUCCESS;
    
}
Result DescriptorMTL::Create(const Texture2DViewDesc& textureViewDesc) {
    TextureMTL& texture = *(TextureMTL*)textureViewDesc.texture;
    const Mip_t remainingMips = textureViewDesc.mipNum - textureViewDesc.mipOffset;
    const Dim_t remainingLayers = textureViewDesc.layerNum - textureViewDesc.layerOffset;
    
    NSRange level;
    level.location = textureViewDesc.mipOffset;
    level.length = textureViewDesc.mipNum == REMAINING_MIPS ? remainingMips:  textureViewDesc.mipNum;
    NSRange slices;
    slices.location = textureViewDesc.layerNum;
    slices.length = textureViewDesc.layerNum == REMAINING_LAYERS ? remainingLayers : textureViewDesc.layerNum;
    m_Texture = [texture.GetHandle()
                 newTextureViewWithPixelFormat: GetFormatMTL(textureViewDesc.format)
                 textureType: MTLTextureType2D
                 levels:level
                 slices:slices];
    
    return Result::SUCCESS;
}
Result DescriptorMTL::Create(const Texture3DViewDesc& textureViewDesc){
    TextureMTL& texture = *(TextureMTL*)textureViewDesc.texture;
    const Mip_t remainingMips = textureViewDesc.mipNum - textureViewDesc.mipOffset;
    const Dim_t remainingLayers = textureViewDesc.sliceNum - textureViewDesc.sliceOffset;
    
    NSRange level;
    level.location = textureViewDesc.mipOffset;
    level.length = textureViewDesc.mipNum == REMAINING_MIPS ? remainingMips:  textureViewDesc.mipNum;
    NSRange slices;
    slices.location = textureViewDesc.sliceOffset;
    slices.length = textureViewDesc.sliceNum == REMAINING_LAYERS ? remainingLayers : textureViewDesc.sliceOffset;
    m_Texture = [texture.GetHandle()
                 newTextureViewWithPixelFormat: GetFormatMTL(textureViewDesc.format)
                 textureType: MTLTextureType3D
                 levels:level
                 slices:slices];
    
    return Result::SUCCESS;
}

Result DescriptorMTL::Create(const SamplerDesc& samplerDesc){
    MTLSamplerDescriptor* mtlDesc = [[MTLSamplerDescriptor alloc] init];
    [mtlDesc setMagFilter: GetFilter(samplerDesc.filters.mag)];
    [mtlDesc setMinFilter: GetFilter(samplerDesc.filters.min)];
    [mtlDesc setRAddressMode: GetSamplerAddressMode(samplerDesc.addressModes.w)];
    [mtlDesc setSAddressMode: GetSamplerAddressMode(samplerDesc.addressModes.u)];
    [mtlDesc setTAddressMode: GetSamplerAddressMode(samplerDesc.addressModes.v)];
    [mtlDesc setMaxAnisotropy: samplerDesc.anisotropy];
    [mtlDesc setCompareFunction: GetCompareOp(samplerDesc.compareFunc)];
    [mtlDesc setLodMinClamp: samplerDesc.mipMin];
    [mtlDesc setLodMaxClamp: samplerDesc.mipMax];
    m_SamplerState = [m_Device newSamplerStateWithDescriptor: mtlDesc];
    
    return Result::SUCCESS;
}

}
