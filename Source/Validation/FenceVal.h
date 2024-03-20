// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct CommandQueueVal;

struct FenceVal : public DeviceObjectVal<Fence> {
    inline FenceVal(DeviceVal& device, Fence* queueSemaphore) : DeviceObjectVal(device, queueSemaphore) {
    }

    inline ~FenceVal() {
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    uint64_t GetFenceValue() const;
    void Wait(uint64_t value);
    void SetDebugName(const char* name);
};

} // namespace nri
