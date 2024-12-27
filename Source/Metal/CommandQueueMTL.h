// © 2021 NVIDIA Corporation
#pragma once

#import <MetalKit/MetalKit.h>

namespace nri {

struct DeviceMTL;

struct CommandQueueMTL {

    inline CommandQueueMTL(DeviceMTL& device)
        : m_Device(device) {
    }

    ~CommandQueueMTL();

    inline DeviceMTL& GetDevice() const {
        return m_Device;
    }

    inline CommandQueueType GetType() const {
        return m_Type;
    }

    inline Lock& GetLock() {
        return m_Lock;
    }
    
    inline id<MTLCommandQueue> GetHandle() const {
        return m_Handle;
    }
    
        
    void SetDebugName(const char* name);
    void Submit(const QueueSubmitDesc& queueSubmitDesc, const SwapChain* swapChain);
    Result WaitForIdle();
   
    Result Create(CommandQueueType type);
    
private:
    DeviceMTL& m_Device;
    CommandQueueType m_Type = CommandQueueType(-1);
    id<MTLCommandQueue> m_Handle;
    Lock m_Lock;

};

}


