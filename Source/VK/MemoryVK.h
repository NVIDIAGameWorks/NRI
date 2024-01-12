// © 2021 NVIDIA Corporation

#pragma once

#include <limits>

namespace nri
{

struct DeviceVK;
struct BufferVK;
struct TextureVK;

struct MemoryVK
{
    inline MemoryVK(DeviceVK& device) :
        m_Device(device)
    {}

    inline VkDeviceMemory GetHandle(uint32_t nodeIndex) const
    { return m_Handles[nodeIndex]; }

    inline DeviceVK& GetDevice() const
    { return m_Device; }

    inline MemoryType GetType() const
    { return m_Type; }

    inline uint8_t* GetMappedMemory(uint32_t nodeIndex) const
    { return m_MappedMemory[nodeIndex]; }

    ~MemoryVK();

    Result Create(uint32_t nodeMask, const MemoryType memoryType, uint64_t size);
    Result Create(const MemoryVKDesc& memoryDesc);
    Result CreateDedicated(BufferVK& buffer, uint32_t nodeMask);
    Result CreateDedicated(TextureVK& texture, uint32_t nodeMask);

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);

private:
    DeviceVK& m_Device;
    std::array<VkDeviceMemory, PHYSICAL_DEVICE_GROUP_MAX_SIZE> m_Handles = {};
    std::array<uint8_t*, PHYSICAL_DEVICE_GROUP_MAX_SIZE> m_MappedMemory = {};
    MemoryType m_Type = std::numeric_limits<MemoryType>::max();
    bool m_OwnsNativeObjects = false;
};

}
