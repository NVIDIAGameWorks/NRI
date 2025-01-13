// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct CommandQueueVal;

struct FenceVal final : public ObjectVal {
    inline FenceVal(DeviceVal& device, Fence* fence)
        : ObjectVal(device, fence) {
    }

    inline ~FenceVal() {
    }

    inline Fence* GetImpl() const {
        return (Fence*)m_Impl;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    uint64_t GetFenceValue() const;
    void Wait(uint64_t value);
};

} // namespace nri
