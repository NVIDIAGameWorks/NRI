// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceMTL;

//struct DescriptorView {
//};

enum class DescriptorTypeMTL {
  NONE,
  IMAGE_VIEW_1D,
  IMAGE_VIEW_2D,
  IMAGE_VIEW_3D,
  SAMPLER,
  BUFFER_VIEW
};

struct DescriptorMTL {
public:
    inline DescriptorMTL (DeviceMTL& device)
        : m_Device(device) {
    }
    
    ~DescriptorMTL();
    inline DescriptorTypeMTL GetType() {
        return m_Type;
    }
    inline id<MTLTexture> GetTextureHandle() {
        return m_Texture;
    }
    inline id<MTLBuffer> GetBufferHandle() {
        return m_Buffer;
    }
    inline id<MTLSamplerState> GetSamplerStateHandler() {
        return m_SamplerState;
    }
    
    inline struct BufferViewDesc& BufferView() {
        return m_BufferViewDesc;
    }
        
    inline DeviceMTL& GetDevice() const {
        return m_Device;
    }
    
    Result Create(const BufferViewDesc& bufferViewDesc);
    Result Create(const Texture1DViewDesc& textureViewDesc);
    Result Create(const Texture2DViewDesc& textureViewDesc);
    Result Create(const Texture3DViewDesc& textureViewDesc);
    Result Create(const SamplerDesc& samplerDesc);

private:
    
    void EndCurrentEncoders(bool forceBarrier);
    
    DeviceMTL& m_Device;
    DescriptorTypeMTL m_Type = DescriptorTypeMTL::NONE;
    id<MTLTexture> m_Texture;
    id<MTLBuffer> m_Buffer;
    id<MTLSamplerState> m_SamplerState;
    union {
        BufferViewDesc m_BufferViewDesc;
        SamplerDesc m_SamplerViewDesc;
    };
};

} // namespace nri
