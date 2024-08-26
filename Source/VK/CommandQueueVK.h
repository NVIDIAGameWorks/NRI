// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceVK;

struct CommandQueueVK {
    inline CommandQueueVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline operator VkQueue() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline uint32_t GetFamilyIndex() const {
        return m_FamilyIndex;
    }

    inline CommandQueueType GetType() const {
        return m_Type;
    }

    inline Lock& GetLock() {
        return m_Lock;
    }

    Result Create(CommandQueueType type, uint32_t familyIndex, VkQueue handle);

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    void Submit(const QueueSubmitDesc& queueSubmitDesc, const SwapChain* swapChain);
    Result UploadData(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum);
    Result WaitForIdle();

private:
    DeviceVK& m_Device;
    VkQueue m_Handle = VK_NULL_HANDLE;
    uint32_t m_FamilyIndex = INVALID_FAMILY_INDEX;
    CommandQueueType m_Type = CommandQueueType(-1);
    Lock m_Lock;
};

} // namespace nri
