// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceMTL;

struct TextureMTL {

    inline TextureMTL(DeviceMTL& device)
        : m_Device(device) {
    }

    ~TextureMTL();
    
    inline id<MTLTexture>& GetHandle() const {
        return m_Handle;
    }

    inline DeviceMTL& GetDevice() const {
        return m_Device;
    }


    Result Create(const TextureDesc& textureDesc);
    Result Create(const TextureMTLDesc& textureDesc);
//    Result Create(const AllocateTextureDesc& textureDesc);

private:
    DeviceMTL& m_Device;
    id<MTLTexture> m_Handle;
    TextureDesc m_Desc = {};
};
}

