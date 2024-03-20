// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceVK;
struct MemoryVK;

struct BufferVK {
    inline BufferVK(DeviceVK& device) : m_Device(device) {
    }

    inline VkBuffer GetHandle() const {
        return m_Handle;
    }

    inline VkDeviceAddress GetDeviceAddress() const {
        return m_DeviceAddress;
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
    VkBuffer m_Handle = VK_NULL_HANDLE;
    VkDeviceAddress m_DeviceAddress = {};
    BufferDesc m_Desc = {};
    MemoryVK* m_Memory = nullptr;
    uint64_t m_MappedMemoryOffset = 0;
    uint64_t m_MappedRangeOffset = 0;
    uint64_t m_MappedRangeSize = 0;
    bool m_OwnsNativeObjects = false;
};

inline VkDeviceAddress GetBufferDeviceAddress(const Buffer* buffer) {
    const BufferVK* bufferVK = (const BufferVK*)buffer;

    return bufferVK != nullptr ? bufferVK->GetDeviceAddress() : 0;
}

} // namespace nri