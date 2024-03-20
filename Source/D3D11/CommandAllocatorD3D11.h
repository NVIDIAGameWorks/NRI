// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceD3D11;

struct CommandAllocatorD3D11 {
    inline CommandAllocatorD3D11(DeviceD3D11& device) : m_Device(device) {
    }

    inline ~CommandAllocatorD3D11() {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name) {
        MaybeUnused(name);
    }

    inline void Reset() {
    }

    Result CreateCommandBuffer(CommandBuffer*& commandBuffer);

private:
    DeviceD3D11& m_Device;
};

} // namespace nri

nri::Result CreateCommandBuffer(nri::DeviceD3D11& deviceImpl, ID3D11DeviceContext* precreatedContext, nri::CommandBuffer*& commandBuffer);
