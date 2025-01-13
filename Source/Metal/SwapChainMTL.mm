#include "SharedMTL.h"

#include "SwapChainMTL.h"
#include "TextureMTL.h"
#include "CommandQueueMTL.h"

using namespace nri;


SwapChainMTL::~SwapChainMTL() {
    
}

void SwapChainMTL::SetDebugName(const char* name) {
    
}

Texture* const* SwapChainMTL::GetTextures(uint32_t& textureNum) const {
    return NULL;
}

uint32_t SwapChainMTL::AcquireNextTexture() {
    m_MTKDrawable = [m_MetalLayer nextDrawable];
    m_FrameIndex = (m_FrameIndex + 1) % m_Textures.size();
    m_Textures[m_FrameIndex]->Create(m_MTKDrawable.texture);
    return m_FrameIndex;
}
Result SwapChainMTL::WaitForPresent() {
    return Result::SUCCESS;
}
Result SwapChainMTL::Present() {
    
    // after committing a command buffer no more commands can be encoded on it: create a new command buffer for future commands
    m_PresentCommandBuffer = [m_CommandQueue->GetHandle() commandBuffer];
    m_PresentCommandBuffer.label = @"PRESENT";

    [m_PresentCommandBuffer presentDrawable: m_MTKDrawable];
    m_MTKDrawable = nil;

    [m_PresentCommandBuffer commit];
    m_PresentCommandBuffer = nil;
    
    return Result::SUCCESS;
}

Result SwapChainMTL::Create(const SwapChainDesc& swapChainDesc) {
    m_MetalLayer = (CAMetalLayer*)swapChainDesc.window.metal.caMetalLayer;
    m_CommandQueue = (CommandQueueMTL*)swapChainDesc.commandQueue;
    m_PresentCommandBuffer = [m_CommandQueue->GetHandle() commandBuffer];
    
//    m_Textures.resize(swapChainDesc.textureNum);
    for(uint32_t i = 0; i < swapChainDesc.textureNum; i++) {
        TextureDesc desc = {};
        desc.width = swapChainDesc.width;
        desc.height = swapChainDesc.height;
        desc.depth = 1;
        desc.mipNum = 1;
        desc.layerNum = 1;
        desc.sampleNum = 1;

        TextureMTL* texture = Allocate<TextureMTL>(m_Device.GetStdAllocator(), m_Device);
        texture->Create(desc);
    }
}
