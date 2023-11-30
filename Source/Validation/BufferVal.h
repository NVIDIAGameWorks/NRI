/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

namespace nri {

struct MemoryVal;

struct BufferVal final : public DeviceObjectVal<Buffer> {
    BufferVal(DeviceVal& device, Buffer* buffer) : DeviceObjectVal(device, buffer) {
    }

    ~BufferVal();

    inline const BufferDesc& GetDesc() const {
        return GetCoreInterface().GetBufferDesc(*GetImpl());
    }

    inline uint64_t GetNativeObject(uint32_t nodeIndex) const {
        return GetCoreInterface().GetBufferNativeObject(*GetImpl(), nodeIndex);
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
    void* Map(uint64_t offset, uint64_t size);
    void Unmap();

  private:
    MemoryVal* m_Memory = nullptr;
    bool m_IsBoundToMemory = false;
    bool m_IsMapped = false;
};

} // namespace nri
