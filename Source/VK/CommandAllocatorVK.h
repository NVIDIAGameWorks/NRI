// © 2021 NVIDIA Corporation

#pragma once

namespace nri
{

struct DeviceVK;

struct CommandAllocatorVK
{
    inline CommandAllocatorVK(DeviceVK& device) :
        m_Device(device)
    {}

    inline operator VkCommandPool() const
    { return m_Handle; }

    inline DeviceVK& GetDevice() const
    { return m_Device; }

    ~CommandAllocatorVK();

    Result Create(const CommandQueue& commandQueue);
    Result Create(const CommandAllocatorVKDesc& commandAllocatorDesc);

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    Result CreateCommandBuffer(CommandBuffer*& commandBuffer);
    void Reset();

private:
    DeviceVK& m_Device;
    VkCommandPool m_Handle = VK_NULL_HANDLE;
    CommandQueueType m_Type = (CommandQueueType)0;
    bool m_OwnsNativeObjects = false;
};

}
