// © 2021 NVIDIA Corporation

#include "SharedVK.h"

#include "AccelerationStructureVK.h"
#include "BufferVK.h"
#include "CommandQueueVK.h"
#include "ConversionVK.h"
#include "DescriptorVK.h"

using namespace nri;

AccelerationStructureVK::~AccelerationStructureVK() {
    if (m_OwnsNativeObjects) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.DestroyAccelerationStructureKHR(m_Device, m_Handle, m_Device.GetAllocationCallbacks());

        Destroy(m_Buffer);
    }
}

Result AccelerationStructureVK::Create(const AccelerationStructureDesc& accelerationStructureDesc) {
    VkAccelerationStructureBuildSizesInfoKHR sizesInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    m_Device.GetAccelerationStructureBuildSizesInfo(accelerationStructureDesc, sizesInfo);

    BufferDesc bufferDesc = {};
    bufferDesc.size = sizesInfo.accelerationStructureSize;
    bufferDesc.usageMask = BufferUsageBits::RAY_TRACING_BUFFER;

    Buffer* buffer = nullptr;
    Result result = m_Device.CreateImplementation<BufferVK>(buffer, bufferDesc);
    if (result == Result::SUCCESS) {
        m_Buffer = (BufferVK*)buffer;
        m_BuildScratchSize = sizesInfo.buildScratchSize;
        m_UpdateScratchSize = sizesInfo.updateScratchSize;
        m_Type = GetAccelerationStructureType(accelerationStructureDesc.type);
        m_AccelerationStructureSize = sizesInfo.accelerationStructureSize;
    }

    return result;
}

Result AccelerationStructureVK::Create(const AccelerationStructureVKDesc& accelerationStructureDesc) {
    if (!accelerationStructureDesc.vkAccelerationStructure)
        return Result::INVALID_ARGUMENT;

    m_OwnsNativeObjects = false;
    m_Handle = (VkAccelerationStructureKHR)accelerationStructureDesc.vkAccelerationStructure;
    m_BuildScratchSize = accelerationStructureDesc.buildScratchSize;
    m_UpdateScratchSize = accelerationStructureDesc.updateScratchSize;

    // Device address
    if (m_Device.m_IsDeviceAddressSupported) {
        VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
        deviceAddressInfo.accelerationStructure = (VkAccelerationStructureKHR)accelerationStructureDesc.vkAccelerationStructure;

        const auto& vk = m_Device.GetDispatchTable();
        m_DeviceAddress = vk.GetAccelerationStructureDeviceAddressKHR(m_Device, &deviceAddressInfo);
    }

    return m_DeviceAddress ? Result::SUCCESS : Result::FAILURE;
}

Result AccelerationStructureVK::FinishCreation() {
    if (!m_Buffer)
        return Result::FAILURE;

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    accelerationStructureCreateInfo.type = m_Type;
    accelerationStructureCreateInfo.size = m_AccelerationStructureSize;
    accelerationStructureCreateInfo.buffer = m_Buffer->GetHandle();

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateAccelerationStructureKHR(m_Device, &accelerationStructureCreateInfo, m_Device.GetAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateAccelerationStructureKHR returned %d", (int32_t)result);

    // Device address
    if (m_Device.m_IsDeviceAddressSupported) {
        VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
        deviceAddressInfo.accelerationStructure = m_Handle;
        m_DeviceAddress = vk.GetAccelerationStructureDeviceAddressKHR(m_Device, &deviceAddressInfo);
    }

    return m_DeviceAddress ? Result::SUCCESS : Result::FAILURE;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void AccelerationStructureVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, (uint64_t)m_Handle, name);

    if (m_Buffer)
        m_Buffer->SetDebugName(name);
}

inline Result AccelerationStructureVK::CreateDescriptor(Descriptor*& descriptor) const {
    DescriptorVK* descriptorImpl = Allocate<DescriptorVK>(m_Device.GetStdAllocator(), m_Device);

    Result result = descriptorImpl->Create(m_Handle);

    if (result == Result::SUCCESS) {
        descriptor = (Descriptor*)descriptorImpl;
        return Result::SUCCESS;
    }

    Destroy(m_Device.GetStdAllocator(), descriptorImpl);

    return Result::SUCCESS;
}

#include "AcceleratrionStructureVK.hpp"