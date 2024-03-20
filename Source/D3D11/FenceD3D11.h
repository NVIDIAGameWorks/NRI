// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceD3D11;
struct CommandQueueD3D11;

struct FenceD3D11 {
    inline FenceD3D11(DeviceD3D11& device) : m_Device(device) {
    }

    inline ~FenceD3D11() {
        if (m_Event != 0 && m_Event != INVALID_HANDLE_VALUE) {
            CloseHandle(m_Event);
        }
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    Result Create(uint64_t initialValue);

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void FenceD3D11::SetDebugName(const char* name) {
        SET_D3D_DEBUG_OBJECT_NAME(m_Fence, name);
        SET_D3D_DEBUG_OBJECT_NAME(m_Query, name);
    }

    uint64_t GetFenceValue() const;
    void QueueSignal(uint64_t value);
    void QueueWait(uint64_t value);
    void Wait(uint64_t value);

private:
    DeviceD3D11& m_Device;
    ComPtr<ID3D11Query> m_Query;
    ComPtr<ID3D11Fence> m_Fence;
    uint64_t m_Value = 0;
    HANDLE m_Event = 0;
};

} // namespace nri
