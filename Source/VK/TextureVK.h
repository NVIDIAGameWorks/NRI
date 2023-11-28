/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

namespace nri
{

struct DeviceVK;

struct TextureVK
{
    inline TextureVK(DeviceVK& device) :
        m_Device(device)
    {}

    inline VkImage GetHandle(uint32_t nodeIndex) const
    { return m_Handles[nodeIndex]; }

    inline DeviceVK& GetDevice() const
    { return m_Device; }

    inline VkImageAspectFlags GetImageAspectFlags() const
    { return m_ImageAspectFlags; }

    inline VkExtent3D GetExtent() const
    { return {m_Desc.width, m_Desc.height, m_Desc.depth}; }

    inline const TextureDesc& GetDesc() const
    { return m_Desc; }

    inline void ClearHandle()
    {
        for (uint32_t i = 0; i < GetCountOf(m_Handles); i++)
            m_Handles[i] = VK_NULL_HANDLE;
    }

    ~TextureVK();

    Result Create(const TextureDesc& textureDesc);
    Result Create(const TextureVKDesc& textureDesc);
    Dim_t GetSize(Dim_t dimensionIndex, Mip_t mip = 0) const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    void GetMemoryInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;

private:
    DeviceVK& m_Device;
    std::array<VkImage, PHYSICAL_DEVICE_GROUP_MAX_SIZE> m_Handles = {};
    TextureDesc m_Desc = {};
    VkImageAspectFlags m_ImageAspectFlags = (VkImageAspectFlags)0;
    bool m_OwnsNativeObjects = false;
};

}