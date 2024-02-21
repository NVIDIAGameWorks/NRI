// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceVK;
struct CommandQueueVK;

struct FenceVK {
    inline FenceVK(DeviceVK& device) : m_Device(device) {
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    ~FenceVK();

    Result Create(uint64_t initialValue);

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    uint64_t GetFenceValue() const;
    void QueueSignal(CommandQueueVK& commandQueue, uint64_t value);
    void QueueWait(CommandQueueVK& commandQueue, uint64_t value);
    void Wait(uint64_t value);

  private:
    DeviceVK& m_Device;
    VkSemaphore m_Fence = VK_NULL_HANDLE;
};

} // namespace nri
