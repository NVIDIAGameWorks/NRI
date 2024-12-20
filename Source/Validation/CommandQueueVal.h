// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct CommandBufferVal;

struct CommandQueueVal : public DeviceObjectVal<CommandQueue> {
    inline CommandQueueVal(DeviceVal& device, CommandQueue* commandQueue)
        : DeviceObjectVal(device, commandQueue) {
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    void Submit(const QueueSubmitDesc& queueSubmitDesc, const SwapChain* swapChain);

    Result WaitForIdle();
    Result UploadData(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum);
};

} // namespace nri
