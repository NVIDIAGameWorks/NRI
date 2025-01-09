// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceVK;
struct MemoryVK;

struct BufferVK {
    inline BufferVK(DeviceVK& device)
        : m_Device(device) {
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
    Result Create(const AllocateBufferDesc& bufferDesc);
    void FinishMemoryBinding(MemoryVK& memory, uint64_t memoryOffset);
    void DestroyVma();
    void GetMemoryDesc(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    void* Map(uint64_t offset, uint64_t size);
    void Unmap();

private:
    DeviceVK& m_Device;
    VkBuffer m_Handle = VK_NULL_HANDLE;
    VkDeviceAddress m_DeviceAddress = 0;
    uint8_t* m_MappedMemory = nullptr;
    VkDeviceMemory m_NonCoherentDeviceMemory = VK_NULL_HANDLE;
    uint64_t m_MappedMemoryOffset = 0;
    uint64_t m_MappedMemoryRangeSize = 0;
    uint64_t m_MappedMemoryRangeOffset = 0;
    BufferDesc m_Desc = {};
    VmaAllocation_T* m_VmaAllocation = nullptr;
    bool m_OwnsNativeObjects = true;
};

inline VkDeviceAddress GetBufferDeviceAddress(const Buffer* buffer) {
    const BufferVK* bufferVK = (const BufferVK*)buffer;

    return bufferVK != nullptr ? bufferVK->GetDeviceAddress() : 0;
}

} // namespace nri