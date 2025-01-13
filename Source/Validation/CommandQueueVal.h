// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct CommandBufferVal;

struct CommandQueueVal final : public ObjectVal {
    inline CommandQueueVal(DeviceVal& device, CommandQueue* commandQueue)
        : ObjectVal(device, commandQueue) {
    }

    inline CommandQueue* GetImpl() const {
        return (CommandQueue*)m_Impl;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    void BeginAnnotation(const char* name, uint32_t bgra);
    void EndAnnotation();
    void Annotation(const char* name, uint32_t bgra);
    void Submit(const QueueSubmitDesc& queueSubmitDesc, const SwapChain* swapChain);

    Result WaitForIdle();
    Result UploadData(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum);
};

} // namespace nri
