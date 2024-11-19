// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceMTL;

enum class DescriptorTypeMTL {
  NONE,
  IMAGE_VIEW
};

struct DescriptorMTL {
public:
    inline DescriptorMTL (DeviceMTL& device)
        : m_Device(device) {
    }
    
    inline id<MTLTexture> GetImageView() {
        return m_texture;
    }
    
    
    Result Create(const BufferViewDesc& bufferViewDesc);
    Result Create(const Texture1DViewDesc& textureViewDesc);
    Result Create(const Texture2DViewDesc& textureViewDesc);
    Result Create(const Texture3DViewDesc& textureViewDesc);
    Result Create(const SamplerDesc& samplerDesc);
private:
    DeviceMTL& m_Device;
    
    id<MTLTexture> m_texture;
    DescriptorTypeMTL m_type = DescriptorTypeMTL::NONE;

};

} // namespace nri
