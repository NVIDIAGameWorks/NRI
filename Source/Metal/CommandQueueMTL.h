// © 2021 NVIDIA Corporation
#pragma once

#import <MetalKit/MetalKit.h>

namespace nri {

struct DeviceMTL;

NriBits(QueueBarrierBits, uint8_t,
    NONE = 0,
    BARRIER_FLAG_BUFFERS = NriBit(0),
    BARRIER_FLAG_TEXTURES = NriBit(1),
    BARRIER_FLAG_RENDERTARGETS = NriBit(2),
    BARRIER_FLAG_FENCE = NriBit(3));

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

    inline CommandQueueType GetType() const {
        return m_Type;
    }

    inline Lock& GetLock() {
        return m_Lock;
    }
    
        
    void SetDebugName(const char* name);
    void Submit(const QueueSubmitDesc& queueSubmitDesc, const SwapChain* swapChain);
    Result WaitForIdle();
   
    Result Create(CommandQueueType type);
    QueueBarrierBits m_BarrierBits = QueueBarrierBits::NONE;

private:
    DeviceMTL& m_Device;
    CommandQueueType m_Type = CommandQueueType(-1);
    id<MTLCommandQueue> m_Handle;
    Lock m_Lock;

};

}


