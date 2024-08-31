// © 2021 NVIDIA Corporation
#pragma once

namespace nri {

struct DeviceMTL;

struct FenceMTL {
    inline FenceMTL(DeviceMTL& device)
        : m_Device(device) {
    }
    ~FenceMTL();

    inline DeviceMTL& GetDevice() const {
        return m_Device;
    }

    Result Create(uint64_t initialValue);

    //void SetDebugName(const char* name);
    //uint64_t GetFenceValue() const;
    //void Wait(uint64_t value);
private:
    dispatch_semaphore_t m_Handle;
    DeviceMTL& m_Device;
};

} // namespace nri
