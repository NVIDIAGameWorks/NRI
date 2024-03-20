// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct BufferVal;
struct TextureVal;
struct AccelerationStructureVal;

struct MemoryVal : public DeviceObjectVal<Memory> {
    MemoryVal(DeviceVal& device, Memory* memory, uint64_t size, MemoryLocation memoryLocation);
    MemoryVal(DeviceVal& device, Memory* memory, const MemoryD3D12Desc& memoryD3D12Desc);

    bool HasBoundResources();
    void ReportBoundResources();

    inline uint64_t GetSize() const {
        return m_Size;
    }

    inline MemoryLocation GetMemoryLocation() const {
        return m_MemoryLocation;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================
    void BindBuffer(BufferVal& buffer);
    void BindTexture(TextureVal& texture);
    void BindAccelerationStructure(AccelerationStructureVal& accelerationStructure);
    void UnbindBuffer(BufferVal& buffer);
    void UnbindTexture(TextureVal& texture);
    void UnbindAccelerationStructure(AccelerationStructureVal& accelerationStructure);

    void SetDebugName(const char* name);

private:
    std::vector<BufferVal*> m_Buffers;
    std::vector<TextureVal*> m_Textures;
    std::vector<AccelerationStructureVal*> m_AccelerationStructures;
    uint64_t m_Size = 0;
    MemoryLocation m_MemoryLocation = MemoryLocation::MAX_NUM;
    Lock m_Lock;
};

} // namespace nri
