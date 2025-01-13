#pragma once

namespace nri {

struct TextureMTL;
struct CommandQueueMTL;

// Let's keep things simple and hide it under the hood
constexpr uint32_t MAX_NUMBER_OF_FRAMES_IN_FLIGHT = 8;

struct SwapChainMTL: public DisplayDescHelper {
    SwapChainMTL(DeviceMTL& device)
    : m_Device(device),
    m_Textures(device.GetStdAllocator()) {
        
    }
    ~SwapChainMTL();

    inline DeviceMTL& GetDevice() const {
        return m_Device;
    }


    void SetDebugName(const char* name);
    Texture* const* GetTextures(uint32_t& textureNum) const;
    uint32_t AcquireNextTexture();
    Result WaitForPresent();
    Result Present();
    
    Result Create(const SwapChainDesc& swapChainDesc);

private:
    CAMetalLayer*  m_MetalLayer;
    Vector<TextureMTL*> m_Textures;
    
    id<CAMetalDrawable>  m_MTKDrawable;
    id<MTLCommandBuffer> m_PresentCommandBuffer;
    CommandQueueMTL* m_CommandQueue = nullptr;
    
    DeviceMTL& m_Device;
    uint64_t m_PresentId = 0;
    uint32_t m_TextureIndex = 0;
    uint8_t m_FrameIndex = 0; // in flight, not global
};

};
