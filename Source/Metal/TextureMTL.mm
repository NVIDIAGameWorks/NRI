#include "TextureMTL.h"

TextureMTL::~TextureMTL() {
    m_Handle = nil;
}


Result Create(const TextureMTLDesc& textureDesc) {
    m_Handle = texturedesc.texture;
    return Result::SUCCESS;
}

Result Create(const TextureMTLDesc& textureDesc) {
    return Result::SUCCESS;
}

Result Create(const TextureDesc& textureDesc) {
    MTLTextureDescriptor* info = [[MTLTextureDescriptor alloc] init];
    m_Device.FillCreateInfo(textureDesc, info);
    
    m_Handle = [m_Device newTextureWithDescriptor:textureDesc];
    return Result::SUCCESS;
}

