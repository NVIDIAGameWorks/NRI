// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct CommandBufferVal;

struct CommandQueueVal : public DeviceObjectVal<CommandQueue> {
    inline CommandQueueVal(DeviceVal& device, CommandQueue* commandQueue) : DeviceObjectVal(device, commandQueue) {
    }

    //================================================================================================================
    // NRI
    //================================================================================================================
    void SetDebugName(const char* name);
    void Submit(const QueueSubmitDesc& queueSubmitDesc, const SwapChain* swapChain);

    Result WaitForIdle();
    Result UploadData(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum);

private:
    void ProcessValidationCommands(const CommandBufferVal* const* commandBuffers, uint32_t commandBufferNum);
    void ProcessValidationCommandBeginQuery(const uint8_t*& begin, const uint8_t* end);
    void ProcessValidationCommandEndQuery(const uint8_t*& begin, const uint8_t* end);
    void ProcessValidationCommandResetQuery(const uint8_t*& begin, const uint8_t* end);
};

} // namespace nri
