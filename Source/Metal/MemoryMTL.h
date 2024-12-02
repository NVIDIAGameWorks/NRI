// © 2021 NVIDIA Corporation

#pragma once

#include <limits>

#import <MetalKit/MetalKit.h>

namespace nri {

struct DeviceMTL;

struct MemoryMTL {

    inline MemoryMTL (DeviceMTL& device)
        : m_Device(device) {
    }

    inline id<MTLHeap> GetHandle() const {
        return m_Handle;
    }

    inline DeviceMTL& GetDevice() const {
        return m_Device;
    }

    inline MemoryType GetType() const {
        return m_Type;
    }

//    inline uint8_t* GetMappedMemory() const {
//        return m_MappedMemory;
//    }

    ~MemoryMTL();
 
    // Result Create(const MemoryMTLDesc& memoryDesc);
    Result Create(const AllocateMemoryDesc& allocateMemoryDesc);
    // Result CreateDedicated(const BufferVK& buffer);
    // Result CreateDedicated(const TextureVK& texture);

    //================================================================================================================
    // NRI
    //================================================================================================================
    void SetDebugName(const char* name);

private:
    DeviceMTL& m_Device;
    id<MTLHeap> m_Handle;
    MemoryType m_Type;
    float m_Priority = 0.0f;
    bool m_OwnsNativeObjects = true;
};


}

