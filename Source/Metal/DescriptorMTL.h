// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceMTL;


struct DescriptorMTL {

    inline DescriptorMTL (DeviceMTL& device)
        : m_Device(device) {
    }

    inline Result Create(const Texture1DViewDesc& textureViewDesc) {}
    inline Result Create(const Texture2DViewDesc& textureViewDesc){}
    inline Result Create(const Texture3DViewDesc& textureViewDesc){}
    inline Result Create(const SamplerDesc& samplerDesc){}

private:
    DeviceMTL& m_Device;

};

} // namespace nri
