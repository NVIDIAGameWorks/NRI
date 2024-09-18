// © 2021 NVIDIA Corporation

NRI_INLINE void CommandQueueD3D11::Submit(const QueueSubmitDesc& queueSubmitDesc) {
    for (uint32_t i = 0; i < queueSubmitDesc.waitFenceNum; i++) {
        const FenceSubmitDesc& fenceSubmitDesc = queueSubmitDesc.waitFences[i];
        FenceD3D11* fence = (FenceD3D11*)fenceSubmitDesc.fence;
        fence->QueueWait(fenceSubmitDesc.value);
    }

    for (uint32_t i = 0; i < queueSubmitDesc.commandBufferNum; i++) {
        CommandBufferHelper* commandBuffer = (CommandBufferHelper*)queueSubmitDesc.commandBuffers[i];
        commandBuffer->Submit();
    }

    for (uint32_t i = 0; i < queueSubmitDesc.signalFenceNum; i++) {
        const FenceSubmitDesc& fenceSubmitDesc = queueSubmitDesc.signalFences[i];
        FenceD3D11* fence = (FenceD3D11*)fenceSubmitDesc.fence;
        fence->QueueSignal(fenceSubmitDesc.value);
    }
}

NRI_INLINE Result CommandQueueD3D11::UploadData(
    const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    HelperDataUpload helperDataUpload(m_Device.GetCoreInterface(), (Device&)m_Device, (CommandQueue&)*this);

    return helperDataUpload.UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

NRI_INLINE Result CommandQueueD3D11::WaitForIdle() {
    return WaitIdle(m_Device.GetCoreInterface(), (Device&)m_Device, (CommandQueue&)*this);
}
