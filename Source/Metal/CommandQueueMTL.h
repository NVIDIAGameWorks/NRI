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
    
    inline operator id<MTLCommandQueue>() const {
        return m_Handle;
    }

    inline DeviceMTL& GetDevice() const {
        return m_Device;
    }
    
    inline uint32_t GetFamilyIndex() const {
        return m_FamilyIndex;
    }

    inline CommandQueueType GetType() const {
        return m_Type;
    }

    inline Lock& GetLock() {
        return m_Lock;
    }
    
    Result WaitForIdle();
        
    void SetDebugName(const char* name);
    Result Create(CommandQueueType type, uint32_t familyIndex, id<MTLCommandQueue> handle);
private:
    DeviceMTL& m_Device;
    uint32_t m_FamilyIndex = 0;
    CommandQueueType m_Type = CommandQueueType(-1);
    id<MTLCommandQueue> m_Handle;
    Lock m_Lock;
};

}


