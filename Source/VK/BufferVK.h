// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceVK;
struct MemoryVK;

struct BufferVK {
    inline BufferVK(DeviceVK& device) : m_Device(device) {
    }

    inline VkBuffer GetHandle(uint32_t nodeIndex) const {
        return m_Handles[nodeIndex];
    }

    inline VkDeviceAddress GetDeviceAddress(uint32_t nodeIndex) const {
        return m_DeviceAddresses[nodeIndex];
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline const BufferDesc& GetDesc() const {
        return m_Desc;
    }

    ~BufferVK();

    Result Create(const BufferDesc& bufferDesc);
    Result Create(const BufferVKDesc& bufferDesc);
    void SetHostMemory(MemoryVK& memory, uint64_t memoryOffset);
    void ReadDeviceAddress();

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    void GetMemoryInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;
    void* Map(uint64_t offset, uint64_t size);
    void Unmap();

  private:
    DeviceVK& m_Device;
    std::array<VkBuffer, PHYSICAL_DEVICE_GROUP_MAX_SIZE> m_Handles = {};
    std::array<VkDeviceAddress, PHYSICAL_DEVICE_GROUP_MAX_SIZE> m_DeviceAddresses = {};
    BufferDesc m_Desc = {};
    MemoryVK* m_Memory = nullptr;
    uint64_t m_MappedMemoryOffset = 0;
    uint64_t m_MappedRangeOffset = 0;
    uint64_t m_MappedRangeSize = 0;
    bool m_OwnsNativeObjects = false;
};

inline VkDeviceAddress GetBufferDeviceAddress(const Buffer* buffer, uint32_t nodeIndex) {
    const BufferVK* bufferVK = (const BufferVK*)buffer;

    return bufferVK != nullptr ? bufferVK->GetDeviceAddress(nodeIndex) : 0;
}

} // namespace nri