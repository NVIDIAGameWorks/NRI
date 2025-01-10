// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct CommandQueueVal;

struct FenceVal final : public DeviceObjectVal<Fence> {
    inline FenceVal(DeviceVal& device, Fence* fence)
        : DeviceObjectVal(device, fence) {
    }

    inline ~FenceVal() {
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    uint64_t GetFenceValue() const;
    void Wait(uint64_t value);
};

} // namespace nri
