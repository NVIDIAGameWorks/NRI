#include "SharedMTL.h"

#include "TextureMTL.h"
#include "MemoryMTL.h"

using namespace nri;

TextureMTL::~TextureMTL() {
    m_Handle = nil;
    [m_label release];
}

void nri::fillMTLTextureDescriptor(const TextureDesc& textureDesc, MTLTextureDescriptor* info) {
    info.textureType = ::GetImageTypeMTL(textureDesc.type);
    info.pixelFormat = ::GetFormatMTL(textureDesc.format, true);
    info.width = textureDesc.width;
    info.height = textureDesc.height;
    info.depth = textureDesc.depth;
    info.mipmapLevelCount = textureDesc.mipNum;
    info.sampleCount = textureDesc.sampleNum;
    info.arrayLength = textureDesc.layerNum;
}

Result TextureMTL::Create(const TextureDesc& textureDesc) {
    m_Desc = textureDesc;
  
    return Result::SUCCESS;
}


void TextureMTL::FinishMemoryBinding(MemoryMTL& memory, uint64_t memoryOffset) {
    MTLTextureDescriptor* info = [[MTLTextureDescriptor alloc] init];
    fillMTLTextureDescriptor(m_Desc, info);
    m_Handle = [memory.GetHandle() newTextureWithDescriptor:info offset:memoryOffset];
    UpdateLabel();
}

void TextureMTL::UpdateLabel() {
    if(m_Handle && m_Label) {
        [m_Handle setLabel: m_Label];
    }
}

void TextureMTL::SetDebugName(const char* name) {
    m_label = [NSString stringWithUTF8String:name];
    [m_label retain];
    UpdateLabel();
}
