// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct CommandAllocatorVal final : public DeviceObjectVal<CommandAllocator> {
    CommandAllocatorVal(DeviceVal& device, CommandAllocator* commandAllocator)
        : DeviceObjectVal(device, commandAllocator) {
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    Result CreateCommandBuffer(CommandBuffer*& commandBuffer);
    void Reset();
};

} // namespace nri
