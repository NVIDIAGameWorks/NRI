#include "SharedMTL.h"

#include "TextureMTL.h"

using namespace nri;

TextureMTL::~TextureMTL() {
    m_Handle = nil;
}


//Result TextureMTL::Create(const TextureMTLDesc& textureDesc) {
//    m_Handle = texturedesc.texture;
//    return Result::SUCCESS;
//}


Result TextureMTL::Create(const TextureDesc& textureDesc) {
    MTLTextureDescriptor* info = [[MTLTextureDescriptor alloc] init];
    info.textureType = ::GetImageTypeMTL(textureDesc.type);
    info.pixelFormat = ::GetFormatMTL(textureDesc.format, true);
    info.width = textureDesc.width;
    info.height = textureDesc.height;
    info.depth = textureDesc.depth;
    info.mipmapLevelCount = textureDesc.mipNum;
    info.sampleCount = textureDesc.sampleNum;
    info.arrayLength = textureDesc.layerNum;
    
    m_Handle = [m_Device newTextureWithDescriptor:info];
    m_Desc = textureDesc;

    //m_Handle = [m_Device newTextureWithDescriptor:textureDesc];
    return Result::SUCCESS;
}

