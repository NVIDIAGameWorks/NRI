// © 2021 NVIDIA Corporation

#include "SharedVK.h"

#include "BufferVK.h"
#include "CommandQueueVK.h"
#include "MemoryVK.h"

using namespace nri;

BufferVK::~BufferVK() {
    const auto& vk = m_Device.GetDispatchTable();

    if (!m_OwnsNativeObjects)
        return;

    if (m_Memory != nullptr)
        vk.DestroyBuffer(m_Device, m_Handles[0], m_Device.GetAllocationCallbacks());
    else {
        for (uint32_t i = 0; i < GetCountOf(m_Handles); i++) {
            if (m_Handles[i] != VK_NULL_HANDLE)
                vk.DestroyBuffer(m_Device, m_Handles[i], m_Device.GetAllocationCallbacks());
        }
    }
}

Result BufferVK::Create(const BufferDesc& bufferDesc) {
    m_OwnsNativeObjects = true;
    m_Desc = bufferDesc;

    const VkSharingMode sharingMode = m_Device.IsConcurrentSharingModeEnabledForBuffers() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    const Vector<uint32_t>& queueIndices = m_Device.GetConcurrentSharingModeQueueIndices();

    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = bufferDesc.size;
    info.usage = GetBufferUsageFlags(bufferDesc.usageMask, bufferDesc.structureStride);
    info.sharingMode = sharingMode;
    info.queueFamilyIndexCount = (uint32_t)queueIndices.size();
    info.pQueueFamilyIndices = queueIndices.data();

    const auto& vk = m_Device.GetDispatchTable();

    const uint32_t nodeMask = GetNodeMask(bufferDesc.nodeMask);

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++) {
        if ((1 << i) & nodeMask) {
            const VkResult result = vk.CreateBuffer(m_Device, &info, m_Device.GetAllocationCallbacks(), &m_Handles[i]);

            RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "Can't create a buffer: vkCreateBuffer returned %d.", (int32_t)result);
        }
    }

    return Result::SUCCESS;
}

Result BufferVK::Create(const BufferVKDesc& bufferDesc) {
    m_OwnsNativeObjects = false;
    m_Memory = (MemoryVK*)bufferDesc.memory;
    m_MappedMemoryOffset = bufferDesc.memoryOffset;

    m_Desc.size = bufferDesc.size;
    m_Desc.structureStride = bufferDesc.structureStride;
    // TODO: m_Desc.usageMask
    m_Desc.nodeMask = GetNodeMask(bufferDesc.nodeMask);

    if (m_Memory != nullptr)
        m_Desc.nodeMask = 0x1;

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++) {
        if ((1 << i) & m_Desc.nodeMask) {
            m_Handles[i] = (VkBuffer)bufferDesc.vkBuffer;
            m_DeviceAddresses[i] = (VkDeviceAddress)bufferDesc.deviceAddress;
        }
    }

    return Result::SUCCESS;
}

void BufferVK::SetHostMemory(MemoryVK& memory, uint64_t memoryOffset) {
    m_Memory = &memory;
    m_MappedMemoryOffset = memoryOffset;

    // No need to keep more than one instance of host buffer
    const auto& vk = m_Device.GetDispatchTable();
    for (uint32_t i = 1; i < m_Device.GetPhysicalDeviceGroupSize(); i++) {
        if (m_Handles[i] != VK_NULL_HANDLE)
            vk.DestroyBuffer(m_Device, m_Handles[i], m_Device.GetAllocationCallbacks());
        m_Handles[i] = m_Handles[0];
    }
}

void BufferVK::ReadDeviceAddress() {
    VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {};
    bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;

    const auto& vk = m_Device.GetDispatchTable();
    if (!vk.GetBufferDeviceAddress)
        return;

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++) {
        if (m_Handles[i] != VK_NULL_HANDLE) {
            bufferDeviceAddressInfo.buffer = m_Handles[i];
            m_DeviceAddresses[i] = vk.GetBufferDeviceAddress(m_Device, &bufferDeviceAddressInfo);
        }
    }
}

//================================================================================================================
// NRI
//================================================================================================================

void BufferVK::SetDebugName(const char* name) {
    std::array<uint64_t, PHYSICAL_DEVICE_GROUP_MAX_SIZE> handles;
    for (size_t i = 0; i < handles.size(); i++)
        handles[i] = (uint64_t)m_Handles[i];

    m_Device.SetDebugNameToDeviceGroupObject(VK_OBJECT_TYPE_BUFFER, handles.data(), name);
}

void BufferVK::GetMemoryInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    VkBuffer handle = VK_NULL_HANDLE;
    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize() && handle == VK_NULL_HANDLE; i++)
        handle = m_Handles[i];

    const auto& vk = m_Device.GetDispatchTable();

    VkMemoryDedicatedRequirements dedicatedRequirements = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS, nullptr};

    VkMemoryRequirements2 requirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2, &dedicatedRequirements};

    VkBufferMemoryRequirementsInfo2 info = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2, nullptr, handle};

    vk.GetBufferMemoryRequirements2(m_Device, &info, &requirements);

    memoryDesc.mustBeDedicated = dedicatedRequirements.requiresDedicatedAllocation;
    memoryDesc.alignment = (uint32_t)requirements.memoryRequirements.alignment;
    memoryDesc.size = requirements.memoryRequirements.size;

    MemoryTypeUnpack unpack = {};
    bool found = m_Device.GetMemoryType(memoryLocation, requirements.memoryRequirements.memoryTypeBits, unpack.info);
    RETURN_ON_FAILURE(&m_Device, found, ReturnVoid(), "Can't find suitable memory type");

    unpack.info.isDedicated = dedicatedRequirements.requiresDedicatedAllocation;

    memoryDesc.type = unpack.type;
}

inline void* BufferVK::Map(uint64_t offset, uint64_t size) {
    CHECK(m_Memory != nullptr, "The buffer does not support memory mapping.");

    m_MappedRangeOffset = offset;
    m_MappedRangeSize = size;

    if (size == WHOLE_SIZE)
        size = m_Desc.size;

    return m_Memory->GetMappedMemory(0) + m_MappedMemoryOffset + offset; // TODO: mGPU + DEVICE_UPLOAD requires massaging
}

inline void BufferVK::Unmap() {
    // TODO: flush memory range if not HOST_COHERENT
    // if (m_Memory->IsHostCoherent())
    //     m_Memory->FlushMemoryRange(m_MappedMemoryOffset + m_MappedRangeOffset, m_MappedRangeSize);
}

#include "BufferVK.hpp"