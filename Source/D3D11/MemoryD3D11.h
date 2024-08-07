// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceD3D11;

enum class MemoryResidencyPriority {
    DEFAULT,
    MINIMUM,
    LOW,
    NORMAL,
    HIGH,
    MAXIMUM
};

struct MemoryD3D11 {
    MemoryD3D11(DeviceD3D11& device, const AllocateMemoryDesc& allocateMemoryDesc);

    inline ~MemoryD3D11() {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    inline MemoryLocation GetLocation() const {
        return m_Location;
    }

    inline uint32_t GetPriority() const {
        return m_Priority;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name) {
        MaybeUnused(name);
    }

private:
    DeviceD3D11& m_Device;
    MemoryLocation m_Location = MemoryLocation::DEVICE;
    uint32_t m_Priority = 0;
};

} // namespace nri
