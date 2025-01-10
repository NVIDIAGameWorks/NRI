// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct MemoryVal;

struct TextureVal final : public DeviceObjectVal<Texture> {
    TextureVal(DeviceVal& device, Texture* texture, bool isBoundToMemory)
        : DeviceObjectVal(device, texture)
        , m_IsBoundToMemory(isBoundToMemory) {
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

    inline void SetBoundToMemory(MemoryVal* memory = nullptr) {
        m_Memory = memory;
        m_IsBoundToMemory = true;
    }

private:
    MemoryVal* m_Memory = nullptr;
    bool m_IsBoundToMemory = false;
};

} // namespace nri
