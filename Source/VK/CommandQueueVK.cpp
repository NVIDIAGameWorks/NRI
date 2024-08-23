// © 2021 NVIDIA Corporation

#include "SharedVK.h"

#include "CommandBufferVK.h"
#include "CommandQueueVK.h"
#include "FenceVK.h"
#include "HelperDataUpload.h"
#include "SwapChainVK.h"

using namespace nri;

Result CommandQueueVK::Create(CommandQueueType type, uint32_t familyIndex, VkQueue handle) {
    m_Type = type;
    m_FamilyIndex = familyIndex;
    m_Handle = handle;

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void CommandQueueVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_QUEUE, (uint64_t)m_Handle, name);
}

inline void CommandQueueVK::Submit(const QueueSubmitDesc& queueSubmitDesc, const SwapChain* swapChain) {
    ExclusiveScope lock(m_Lock);

    VkSemaphoreSubmitInfo* waitSemaphores = StackAlloc(VkSemaphoreSubmitInfo, queueSubmitDesc.waitFenceNum);
    for (uint32_t i = 0; i < queueSubmitDesc.waitFenceNum; i++) {
        waitSemaphores[i] = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        waitSemaphores[i].semaphore = *(FenceVK*)queueSubmitDesc.waitFences[i].fence;
        waitSemaphores[i].value = queueSubmitDesc.waitFences[i].value;
        waitSemaphores[i].stageMask = GetPipelineStageFlags(queueSubmitDesc.waitFences[i].stages);
    }

    VkCommandBufferSubmitInfo* commandBuffers = StackAlloc(VkCommandBufferSubmitInfo, queueSubmitDesc.commandBufferNum);
    for (uint32_t i = 0; i < queueSubmitDesc.commandBufferNum; i++) {
        commandBuffers[i] = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
        commandBuffers[i].commandBuffer = *(CommandBufferVK*)queueSubmitDesc.commandBuffers[i];
    }

    VkSemaphoreSubmitInfo* signalSemaphores = StackAlloc(VkSemaphoreSubmitInfo, queueSubmitDesc.signalFenceNum);
    for (uint32_t i = 0; i < queueSubmitDesc.signalFenceNum; i++) {
        signalSemaphores[i] = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        signalSemaphores[i].semaphore = *(FenceVK*)queueSubmitDesc.signalFences[i].fence;
        signalSemaphores[i].value = queueSubmitDesc.signalFences[i].value;
        signalSemaphores[i].stageMask = GetPipelineStageFlags(queueSubmitDesc.signalFences[i].stages);
    }

    VkSubmitInfo2 submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submitInfo.waitSemaphoreInfoCount = queueSubmitDesc.waitFenceNum;
    submitInfo.pWaitSemaphoreInfos = waitSemaphores;
    submitInfo.commandBufferInfoCount = queueSubmitDesc.commandBufferNum;
    submitInfo.pCommandBufferInfos = commandBuffers;
    submitInfo.signalSemaphoreInfoCount = queueSubmitDesc.signalFenceNum;
    submitInfo.pSignalSemaphoreInfos = signalSemaphores;

    VkLatencySubmissionPresentIdNV presentId = {VK_STRUCTURE_TYPE_LATENCY_SUBMISSION_PRESENT_ID_NV};
    if (swapChain && m_Device.m_IsPresentIdSupported) {
        presentId.presentID = ((const SwapChainVK*)swapChain)->GetPresentId();
        submitInfo.pNext = &presentId;
    }

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.QueueSubmit2(m_Handle, 1, &submitInfo, VK_NULL_HANDLE);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, ReturnVoid(), "Submit: vkQueueSubmit returned %d", (int32_t)result);
}

inline Result CommandQueueVK::UploadData(
    const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    HelperDataUpload helperDataUpload(m_Device.GetCoreInterface(), (Device&)m_Device, (CommandQueue&)*this);

    return helperDataUpload.UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

inline Result CommandQueueVK::WaitForIdle() {
    ExclusiveScope lock(m_Lock);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.QueueWaitIdle(m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "WaitForIdle: vkQueueWaitIdle returned %d", (int32_t)result);

    return Result::SUCCESS;
}

#include "CommandQueueVK.hpp"