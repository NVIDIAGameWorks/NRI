// © 2021 NVIDIA Corporation

#pragma once

#ifdef NRI_USE_AGILITY_SDK
struct ID3D12Resource2;
typedef ID3D12Resource2 ID3D12ResourceBest;
#else
struct ID3D12Resource;
typedef ID3D12Resource ID3D12ResourceBest;
#endif

namespace nri {

struct DeviceD3D12;
struct MemoryD3D12;

struct BufferD3D12 {
    inline BufferD3D12(DeviceD3D12& device) : m_Device(device) {
    }

    inline ~BufferD3D12() {
    }

    inline operator ID3D12ResourceBest*() const {
        return m_Buffer.GetInterface();
    }

    inline const BufferDesc& GetDesc() const {
        return m_Desc;
    }

    inline D3D12_GPU_VIRTUAL_ADDRESS GetPointerGPU() const {
        return m_Buffer->GetGPUVirtualAddress();
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    Result Create(const BufferDesc& bufferDesc);
    Result Create(const BufferD3D12Desc& bufferDesc);
    Result BindMemory(const MemoryD3D12* memory, uint64_t offset, bool isAccelerationStructureBuffer = false);

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name) {
        SET_D3D_DEBUG_OBJECT_NAME(m_Buffer, name);
    }

    void GetMemoryInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;
    void* Map(uint64_t offset, uint64_t size);
    void Unmap();

  private:
    DeviceD3D12& m_Device;
    ComPtr<ID3D12ResourceBest> m_Buffer;
    BufferDesc m_Desc = {};
};

} // namespace nri
