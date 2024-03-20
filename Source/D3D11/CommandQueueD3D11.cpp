// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"

#include "CommandQueueD3D11.h"
#include "FenceD3D11.h"
#include "HelperDataUpload.h"
#include "HelperWaitIdle.h"

using namespace nri;

//================================================================================================================
// NRI
//================================================================================================================

inline void CommandQueueD3D11::Submit(const QueueSubmitDesc& queueSubmitDesc) {
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

inline Result CommandQueueD3D11::UploadData(
    const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    HelperDataUpload helperDataUpload(m_Device.GetCoreInterface(), (Device&)m_Device, (CommandQueue&)*this);

    return helperDataUpload.UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

inline Result CommandQueueD3D11::WaitForIdle() {
    return WaitIdle(m_Device.GetCoreInterface(), (Device&)m_Device, (CommandQueue&)*this);
}

#include "CommandQueueD3D11.hpp"
