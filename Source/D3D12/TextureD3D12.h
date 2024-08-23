// © 2021 NVIDIA Corporation

#pragma once

#ifdef NRI_USE_AGILITY_SDK
struct ID3D12Resource2;
typedef ID3D12Resource2 ID3D12ResourceBest;
#else
struct ID3D12Resource;
typedef ID3D12Resource ID3D12ResourceBest;
#endif

namespace nri {

struct DeviceD3D12;
struct MemoryD3D12;

struct TextureD3D12 {
    inline TextureD3D12(DeviceD3D12& device) : m_Device(device) {
    }

    inline ~TextureD3D12() {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline const TextureDesc& GetDesc() const {
        return m_Desc;
    }

    inline operator ID3D12ResourceBest*() const {
        return m_Texture.GetInterface();
    }

    inline uint32_t GetSubresourceIndex(Dim_t arrayOffset, Mip_t mipOffset) const {
        return arrayOffset * m_Desc.mipNum + mipOffset;
    }

    inline Dim_t GetSize(Dim_t dimensionIndex, Mip_t mip = 0) const {
        return GetDimension(GraphicsAPI::D3D12, m_Desc, dimensionIndex, mip);
    }

    Result Create(const TextureDesc& textureDesc);
    Result Create(const TextureD3D12Desc& textureDesc);
    Result Create(const AllocateTextureDesc& textureDesc);
    Result BindMemory(const MemoryD3D12* memory, uint64_t offset);

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name) {
        SET_D3D_DEBUG_OBJECT_NAME(m_Texture, name);
    }

private:
    DeviceD3D12& m_Device;
    ComPtr<ID3D12ResourceBest> m_Texture;
    ComPtr<D3D12MA::Allocation> m_VmaAllocation = nullptr;
    TextureDesc m_Desc = {};
};

} // namespace nri
