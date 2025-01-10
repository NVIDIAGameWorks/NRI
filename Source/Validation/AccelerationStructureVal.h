// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct MemoryVal;

struct AccelerationStructureVal final : public DeviceObjectVal<AccelerationStructure> {
    AccelerationStructureVal(DeviceVal& device, AccelerationStructure* accelerationStructure, bool isBoundToMemory, const MemoryDesc& memoryDesc)
        : DeviceObjectVal(device, accelerationStructure)
        , m_IsBoundToMemory(isBoundToMemory)
        , m_MemoryDesc(memoryDesc) {
    }

    ~AccelerationStructureVal();

    inline bool IsBoundToMemory() const {
        return m_IsBoundToMemory;
    }

    inline void SetBoundToMemory(MemoryVal& memory) {
        m_Memory = &memory;
        m_IsBoundToMemory = true;
    }

    inline const MemoryDesc& GetMemoryDesc() const {
        return m_MemoryDesc;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    uint64_t GetUpdateScratchBufferSize() const;
    uint64_t GetBuildScratchBufferSize() const;
    uint64_t GetHandle() const;
    uint64_t GetNativeObject() const;
    Result CreateDescriptor(Descriptor*& descriptor);

private:
    MemoryVal* m_Memory = nullptr;
    MemoryDesc m_MemoryDesc = {};
    bool m_IsBoundToMemory = false;
};

} // namespace nri
