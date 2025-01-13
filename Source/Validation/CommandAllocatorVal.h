// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct CommandAllocatorVal final : public ObjectVal {
    CommandAllocatorVal(DeviceVal& device, CommandAllocator* commandAllocator)
        : ObjectVal(device, commandAllocator) {
    }

    inline CommandAllocator* GetImpl() const {
        return (CommandAllocator*)m_Impl;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result CreateCommandBuffer(CommandBuffer*& commandBuffer);
    void Reset();
};

} // namespace nri
