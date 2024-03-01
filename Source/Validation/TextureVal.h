// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct MemoryVal;

struct TextureVal : public DeviceObjectVal<Texture> {
    TextureVal(DeviceVal& device, Texture* texture) : DeviceObjectVal(device, texture) {
    }

    ~TextureVal();

    inline const TextureDesc& GetDesc() const {
        return GetCoreInterface().GetTextureDesc(*GetImpl());
    }

    inline uint64_t GetNativeObject() const {
        return GetCoreInterface().GetTextureNativeObject(*GetImpl());
    }

    inline bool IsBoundToMemory() const {
        return m_IsBoundToMemory;
    }

    inline void SetBoundToMemory() {
        m_IsBoundToMemory = true;
    }

    inline void SetBoundToMemory(MemoryVal& memory) {
        m_Memory = &memory;
        m_IsBoundToMemory = true;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================
    void SetDebugName(const char* name);
    void GetMemoryInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;

  private:
    MemoryVal* m_Memory = nullptr;
    bool m_IsBoundToMemory = false;
};

} // namespace nri
