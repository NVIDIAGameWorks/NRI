// © 2021 NVIDIA Corporation

#include "SharedVK.h"

#include "CommandQueueVK.h"
#include "FenceVK.h"

using namespace nri;

FenceVK::~FenceVK() {
    const auto& vk = m_Device.GetDispatchTable();
    if (m_Fence != VK_NULL_HANDLE)
        vk.DestroySemaphore(m_Device, m_Fence, m_Device.GetAllocationCallbacks());
}

Result FenceVK::Create(uint64_t initialValue) {
    VkSemaphoreTypeCreateInfo timelineCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO, nullptr, VK_SEMAPHORE_TYPE_TIMELINE, initialValue};
    VkSemaphoreCreateInfo createInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, &timelineCreateInfo, 0};

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateSemaphore((VkDevice)m_Device, &createInfo, m_Device.GetAllocationCallbacks(), &m_Fence);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vk.CreateSemaphore returned %d", (int32_t)result);

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void FenceVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)m_Fence, name);
}

inline uint64_t FenceVK::GetFenceValue() const {
    uint64_t value = 0;

    const auto& vk = m_Device.GetDispatchTable();
    vk.GetSemaphoreCounterValue((VkDevice)m_Device, m_Fence, &value);

    return value;
}

inline void FenceVK::QueueSignal(CommandQueueVK& commandQueue, uint64_t value) {
    ExclusiveScope lock(commandQueue.GetLock());

    VkTimelineSemaphoreSubmitInfo timelineInfo = {VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO, nullptr, 0, nullptr, 1, &value};
    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO, &timelineInfo, 0, nullptr, nullptr, 0, nullptr, 1, &m_Fence};

    const auto& vk = m_Device.GetDispatchTable();
    vk.QueueSubmit((VkQueue)commandQueue, 1, &submitInfo, VK_NULL_HANDLE);
}

inline void FenceVK::QueueWait(CommandQueueVK& commandQueue, uint64_t value) {
    ExclusiveScope lock(commandQueue.GetLock());

    VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT; // TODO: matches D3D?
    VkTimelineSemaphoreSubmitInfo timelineInfo = {VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO, nullptr, 1, &value, 0, nullptr};
    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO, &timelineInfo, 1, &m_Fence, &waitDstStageMask, 0, nullptr, 0, nullptr};

    const auto& vk = m_Device.GetDispatchTable();
    vk.QueueSubmit((VkQueue)commandQueue, 1, &submitInfo, VK_NULL_HANDLE);
}

inline void FenceVK::Wait(uint64_t value) {
    VkSemaphoreWaitInfo waitInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO, nullptr, 0, 1, &m_Fence, &value};

    const auto& vk = m_Device.GetDispatchTable();
    vk.WaitSemaphores((VkDevice)m_Device, &waitInfo, VK_DEFAULT_TIMEOUT);
}

#include "FenceVK.hpp"
