// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceVK;
struct BufferVK;

struct AccelerationStructureVK {
    inline AccelerationStructureVK(DeviceVK& device) : m_Device(device) {
    }

    inline VkAccelerationStructureKHR GetHandle() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline BufferVK* GetBuffer() const {
        return m_Buffer;
    }

    inline bool OwnsNativeObjects() const {
        return m_OwnsNativeObjects;
    }

    ~AccelerationStructureVK();

    Result Create(const AccelerationStructureDesc& accelerationStructureDesc);
    Result Create(const AccelerationStructureVKDesc& accelerationStructureDesc);
    Result FinishCreation();

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline uint64_t GetUpdateScratchBufferSize() const {
        return Align(m_UpdateScratchSize, m_Device.GetDesc().rayTracingScratchAlignment);
    }

    inline uint64_t GetBuildScratchBufferSize() const {
        return Align(m_BuildScratchSize, m_Device.GetDesc().rayTracingScratchAlignment);
    }

    inline VkDeviceAddress GetNativeHandle() const {
        return m_DeviceAddress;
    }

    void SetDebugName(const char* name);
    void GetMemoryInfo(MemoryDesc& memoryDesc) const;
    Result CreateDescriptor(Descriptor*& descriptor) const;

private:
    void PrecreateBottomLevel(const AccelerationStructureDesc& accelerationStructureDesc);
    void PrecreateTopLevel(const AccelerationStructureDesc& accelerationStructureDesc);

private:
    DeviceVK& m_Device;
    VkAccelerationStructureKHR m_Handle = VK_NULL_HANDLE;
    VkDeviceAddress m_DeviceAddress = 0;
    BufferVK* m_Buffer = nullptr;
    uint64_t m_BuildScratchSize = 0;
    uint64_t m_UpdateScratchSize = 0;
    uint64_t m_AccelerationStructureSize = 0;
    VkAccelerationStructureTypeKHR m_Type = (VkAccelerationStructureTypeKHR)0;
    VkBuildAccelerationStructureFlagsKHR m_BuildFlags = (VkBuildAccelerationStructureFlagsKHR)0;
    bool m_OwnsNativeObjects = false;
};

} // namespace nri