// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceMTL;
struct CommandQueueMTL;

struct CommandAllocatorMTL {
    inline CommandAllocatorMTL(DeviceMTL& device)
        : m_Device(device) {
    }
    
    inline DeviceMTL& GetDevice() const {
        return m_Device;
    }

    ~CommandAllocatorMTL();


    Result Create(const CommandQueue& commandQueue);

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    Result CreateCommandBuffer(CommandBuffer*& commandBuffer);
    void Reset();

private:
    DeviceMTL& m_Device;
    struct CommandQueueMTL* m_CommandQueue;
};

}

