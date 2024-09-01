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

    inline const TextureDesc& GetDesc() const {
        return m_Desc;
    }

    Result Create(const TextureDesc& textureDesc);
    Result Create(const TextureMTLDesc& textureDesc);

private:
    DeviceMTL& m_Device;
    TextureDesc m_Desc = {};
    id<MTLTexture> m_Handle;
};

} // namespace nri