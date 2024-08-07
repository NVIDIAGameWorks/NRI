// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceD3D12;

struct MemoryD3D12 {
    inline MemoryD3D12(DeviceD3D12& device) : m_Device(device) {
    }

    inline ~MemoryD3D12() {
    }

    inline operator ID3D12Heap*() const {
        return m_Heap.GetInterface();
    }

    inline const D3D12_HEAP_DESC& GetHeapDesc() const {
        return m_HeapDesc;
    }

    inline bool RequiresDedicatedAllocation() const {
        return m_Heap.GetInterface() ? false : true;
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    Result Create(const AllocateMemoryDesc& allocateMemoryDesc);
    Result Create(const MemoryD3D12Desc& memoryDesc);
    void SetPriority(ID3D12Pageable* obj) const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name) {
        SET_D3D_DEBUG_OBJECT_NAME(m_Heap, name);
    }

private:
    DeviceD3D12& m_Device;
    ComPtr<ID3D12Heap> m_Heap;
    D3D12_HEAP_DESC m_HeapDesc = {};
    uint32_t m_Priority = 0;
};

} // namespace nri
