// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct MemoryVal;

struct AccelerationStructureVal final : public DeviceObjectVal<AccelerationStructure> {
    AccelerationStructureVal(DeviceVal& device, AccelerationStructure* accelerationStructure, bool isBoundToMemory)
        : DeviceObjectVal(device, accelerationStructure), m_RayTracingAPI(device.GetRayTracingInterface()), m_IsBoundToMemory(isBoundToMemory) {
    }

    ~AccelerationStructureVal();

    inline bool IsBoundToMemory() const {
        return m_IsBoundToMemory;
    }

    inline void SetBoundToMemory(MemoryVal& memory) {
        m_Memory = &memory;
        m_IsBoundToMemory = true;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================
    void GetMemoryInfo(MemoryDesc& memoryDesc) const;
    uint64_t GetUpdateScratchBufferSize() const;
    uint64_t GetBuildScratchBufferSize() const;
    uint64_t GetHandle(uint32_t nodeIndex) const;
    uint64_t GetNativeObject(uint32_t nodeIndex) const;
    Result CreateDescriptor(uint32_t nodeIndex, Descriptor*& descriptor);
    void SetDebugName(const char* name);

  private:
    const RayTracingInterface& m_RayTracingAPI;
    MemoryVal* m_Memory = nullptr;
    bool m_IsBoundToMemory = false;
};

} // namespace nri
