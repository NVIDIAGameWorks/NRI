// © 2021 NVIDIA Corporation
#pragma once

#import <MetalKit/MetalKit.h>

namespace nri {

struct DeviceMTL;
struct MemoryMTL;

struct BufferMTL {
    
    inline BufferMTL(DeviceMTL& device)
    : m_Device(device) {
    }
    
    ~BufferMTL();
    
    inline id<MTLBuffer> GetHandle() const {
        return m_Handle;
    }
    
    inline DeviceMTL& GetDevice() const {
        return m_Device;
    }
    
    inline const BufferDesc& GetDesc() const {
        return m_Desc;
    }
    
    void* Map(uint64_t offset, uint64_t size);
    void Unmap();
    
    void FinishMemoryBinding(MemoryMTL& memory, uint64_t memoryOffset);
    Result Create(const BufferDesc& bufferDesc);
    
    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);

private:
    void UpdateLabel();
    
    NSString* m_Label = nullptr;
    DeviceMTL& m_Device;
    id<MTLBuffer> m_Handle;
    uint8_t* m_MappedMemory = nullptr;
    uint64_t m_MappedMemoryOffset = 0;
    uint64_t m_MappedMemoryRangeSize = 0;
    uint64_t m_MappedMemoryRangeOffset = 0;
    BufferDesc m_Desc = {};
    bool m_OwnsNativeObjects = true;
};


};
