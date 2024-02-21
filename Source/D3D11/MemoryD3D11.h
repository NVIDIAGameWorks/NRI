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
    inline MemoryD3D11(DeviceD3D11& device, MemoryType memoryType) : m_Device(device), m_Location((MemoryLocation)memoryType) {
    }

    inline ~MemoryD3D11() {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    inline MemoryLocation GetType() const {
        return m_Location;
    }

    uint32_t GetResidencyPriority(uint64_t size) const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name) {
        MaybeUnused(name);
    }

  private:
    DeviceD3D11& m_Device;
    MemoryLocation m_Location;
    MemoryResidencyPriority m_ResidencyPriority = MemoryResidencyPriority::DEFAULT;
};

} // namespace nri
