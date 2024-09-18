// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct MemoryVal;

struct BufferVal final : public DeviceObjectVal<Buffer> {
    BufferVal(DeviceVal& device, Buffer* buffer, bool isBoundToMemory)
        : DeviceObjectVal(device, buffer)
        , m_IsBoundToMemory(isBoundToMemory) {
    }

    ~BufferVal();

    inline const BufferDesc& GetDesc() const {
        return GetCoreInterface().GetBufferDesc(*GetImpl());
    }

    inline uint64_t GetNativeObject() const {
        return GetCoreInterface().GetBufferNativeObject(*GetImpl());
    }

    inline bool IsBoundToMemory() const {
        return m_IsBoundToMemory;
    }

    inline void SetBoundToMemory(MemoryVal* memory = nullptr) {
        m_Memory = memory;
        m_IsBoundToMemory = true;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    void* Map(uint64_t offset, uint64_t size);
    void Unmap();

private:
    MemoryVal* m_Memory = nullptr;
    bool m_IsBoundToMemory = false;
    bool m_IsMapped = false;
};

} // namespace nri
