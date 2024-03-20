// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceVK;

struct TextureVK {
    inline TextureVK(DeviceVK& device) : m_Device(device) {
    }

    inline VkImage GetHandle() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline VkImageAspectFlags GetImageAspectFlags() const {
        return m_ImageAspectFlags;
    }

    inline VkExtent3D GetExtent() const {
        return {m_Desc.width, m_Desc.height, m_Desc.depth};
    }

    inline const TextureDesc& GetDesc() const {
        return m_Desc;
    }

    inline Dim_t GetSize(Dim_t dimensionIndex, Mip_t mip = 0) const {
        return GetDimension(GraphicsAPI::VULKAN, m_Desc, dimensionIndex, mip);
    }

    ~TextureVK();

    Result Create(const TextureDesc& textureDesc);
    Result Create(const TextureVKDesc& textureDesc);

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    void GetMemoryInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;

private:
    DeviceVK& m_Device;
    VkImage m_Handle = VK_NULL_HANDLE;
    TextureDesc m_Desc = {};
    VkImageAspectFlags m_ImageAspectFlags = (VkImageAspectFlags)0;
    bool m_OwnsNativeObjects = false;
};

} // namespace nri