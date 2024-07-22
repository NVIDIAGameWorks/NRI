// © 2021 NVIDIA Corporation

#include "SharedVK.h"

#include "AccelerationStructureVK.h"
#include "BufferVK.h"
#include "CommandQueueVK.h"
#include "ConversionVK.h"
#include "DescriptorVK.h"

using namespace nri;

AccelerationStructureVK::~AccelerationStructureVK() {
    const auto& vk = m_Device.GetDispatchTable();

    if (!m_OwnsNativeObjects)
        return;

    vk.DestroyAccelerationStructureKHR(m_Device, m_Handle, m_Device.GetAllocationCallbacks());

    if (m_Buffer)
        m_Device.DestroyBuffer(*(Buffer*)m_Buffer);
}

Result AccelerationStructureVK::Create(const AccelerationStructureDesc& accelerationStructureDesc) {
    m_OwnsNativeObjects = true;
    m_Type = GetAccelerationStructureType(accelerationStructureDesc.type);
    m_BuildFlags = GetAccelerationStructureBuildFlags(accelerationStructureDesc.flags);

    if (accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL)
        PrecreateBottomLevel(accelerationStructureDesc);
    else
        PrecreateTopLevel(accelerationStructureDesc);

    BufferDesc bufferDesc = {};
    bufferDesc.size = m_AccelerationStructureSize;
    bufferDesc.usageMask = BufferUsageBits::RAY_TRACING_BUFFER;

    Buffer* buffer = nullptr;
    const Result result = m_Device.CreateBuffer(bufferDesc, buffer);
    m_Buffer = (BufferVK*)buffer;

    return result;
}

Result AccelerationStructureVK::Create(const AccelerationStructureVKDesc& accelerationStructureDesc) {
    m_OwnsNativeObjects = false;

    VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    deviceAddressInfo.accelerationStructure = (VkAccelerationStructureKHR)accelerationStructureDesc.vkAccelerationStructure;

    const auto& vk = m_Device.GetDispatchTable();
    const uint64_t deviceAddress = vk.GetAccelerationStructureDeviceAddressKHR(m_Device, &deviceAddressInfo);

    if (deviceAddress == 0)
        return Result::FAILURE;

    m_Type = VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR;
    m_BuildFlags = 0;
    m_Handle = (VkAccelerationStructureKHR)accelerationStructureDesc.vkAccelerationStructure;
    m_DeviceAddress = deviceAddress;
    m_BuildScratchSize = accelerationStructureDesc.buildScratchSize;
    m_UpdateScratchSize = accelerationStructureDesc.updateScratchSize;

    return Result::SUCCESS;
}

void AccelerationStructureVK::PrecreateBottomLevel(const AccelerationStructureDesc& accelerationStructureDesc) {
    const auto buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildInfo.type = m_Type;
    buildInfo.flags = m_BuildFlags;
    buildInfo.geometryCount = accelerationStructureDesc.instanceOrGeometryObjectNum;

    VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};

    Vector<VkAccelerationStructureGeometryKHR> geometries(m_Device.GetStdAllocator());
    uint32_t* primitiveMaxNums = ALLOCATE_SCRATCH(m_Device, uint32_t, accelerationStructureDesc.instanceOrGeometryObjectNum);

    geometries.resize(accelerationStructureDesc.instanceOrGeometryObjectNum);
    ConvertGeometryObjectSizesVK(geometries.data(), primitiveMaxNums, accelerationStructureDesc.geometryObjects, (uint32_t)geometries.size());
    buildInfo.pGeometries = geometries.data();

    const auto& vk = m_Device.GetDispatchTable();
    vk.GetAccelerationStructureBuildSizesKHR(m_Device, buildType, &buildInfo, primitiveMaxNums, &sizeInfo);

    m_BuildScratchSize = sizeInfo.buildScratchSize;
    m_UpdateScratchSize = sizeInfo.updateScratchSize;
    m_AccelerationStructureSize = sizeInfo.accelerationStructureSize;

    FREE_SCRATCH(m_Device, primitiveMaxNums, accelerationStructureDesc.instanceOrGeometryObjectNum);
}

void AccelerationStructureVK::PrecreateTopLevel(const AccelerationStructureDesc& accelerationStructureDesc) {
    const auto buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

    VkAccelerationStructureGeometryKHR geometry = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildInfo.type = m_Type;
    buildInfo.flags = m_BuildFlags;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geometry;

    const uint32_t instanceMaxNum = accelerationStructureDesc.instanceOrGeometryObjectNum;

    VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};

    const auto& vk = m_Device.GetDispatchTable();
    vk.GetAccelerationStructureBuildSizesKHR(m_Device, buildType, &buildInfo, &instanceMaxNum, &sizeInfo);

    m_BuildScratchSize = sizeInfo.buildScratchSize;
    m_UpdateScratchSize = sizeInfo.updateScratchSize;
    m_AccelerationStructureSize = sizeInfo.accelerationStructureSize;
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

    VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    deviceAddressInfo.accelerationStructure = m_Handle;

    m_DeviceAddress = vk.GetAccelerationStructureDeviceAddressKHR(m_Device, &deviceAddressInfo);

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void AccelerationStructureVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, (uint64_t)m_Handle, name);
    m_Buffer->SetDebugName(name);
}

inline void AccelerationStructureVK::GetMemoryDesc(MemoryDesc& memoryDesc) const {
    m_Device.GetMemoryDesc(m_Buffer->GetDesc(), MemoryLocation::DEVICE, memoryDesc);
}

inline Result AccelerationStructureVK::CreateDescriptor(Descriptor*& descriptor) const {
    DescriptorVK& descriptorImpl = *Allocate<DescriptorVK>(m_Device.GetStdAllocator(), m_Device);
    descriptorImpl.Create(m_Handle);
    descriptor = (Descriptor*)&descriptorImpl;

    return Result::SUCCESS;
}

#include "AcceleratrionStructureVK.hpp"