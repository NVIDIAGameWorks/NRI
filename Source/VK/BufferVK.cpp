// © 2021 NVIDIA Corporation

#include "SharedVK.h"

#include "BufferVK.h"
#include "CommandQueueVK.h"
#include "MemoryVK.h"

using namespace nri;

constexpr VkBufferUsageFlags GetBufferUsageFlags(BufferUsageBits bufferUsageBits, uint32_t structureStride) {
    VkBufferUsageFlags flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (bufferUsageBits & BufferUsageBits::VERTEX_BUFFER)
        flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    if (bufferUsageBits & BufferUsageBits::INDEX_BUFFER)
        flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    if (bufferUsageBits & BufferUsageBits::CONSTANT_BUFFER)
        flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    if (bufferUsageBits & BufferUsageBits::ARGUMENT_BUFFER)
        flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

    if (bufferUsageBits & BufferUsageBits::RAY_TRACING_BUFFER) // TODO: add more usage bits?
        flags |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    if (bufferUsageBits & BufferUsageBits::ACCELERATION_STRUCTURE_BUILD_READ)
        flags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

    if (bufferUsageBits & BufferUsageBits::SHADER_RESOURCE) {
        if (structureStride == 0)
            flags |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
        else
            flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    if (bufferUsageBits & BufferUsageBits::SHADER_RESOURCE_STORAGE) {
        if (structureStride == 0 && (bufferUsageBits & BufferUsageBits::RAY_TRACING_BUFFER) == 0)
            flags |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
        else
            flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    return flags;
}

BufferVK::~BufferVK() {
    const auto& vk = m_Device.GetDispatchTable();

    if (m_OwnsNativeObjects)
        vk.DestroyBuffer(m_Device, m_Handle, m_Device.GetAllocationCallbacks());
}

Result BufferVK::Create(const BufferDesc& bufferDesc) {
    m_OwnsNativeObjects = true;
    m_Desc = bufferDesc;

    const VkSharingMode sharingMode = m_Device.IsConcurrentSharingModeEnabledForBuffers() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    const Vector<uint32_t>& queueIndices = m_Device.GetConcurrentSharingModeQueueIndices();

    VkBufferCreateInfo info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = bufferDesc.size;
    info.usage = GetBufferUsageFlags(bufferDesc.usageMask, bufferDesc.structureStride);
    info.sharingMode = sharingMode;
    info.queueFamilyIndexCount = (uint32_t)queueIndices.size();
    info.pQueueFamilyIndices = queueIndices.data();

    if (m_Device.m_IsDeviceAddressSupported)
        info.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateBuffer(m_Device, &info, m_Device.GetAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateBuffer returned %d", (int32_t)result);

    return Result::SUCCESS;
}

Result BufferVK::Create(const BufferVKDesc& bufferDesc) {
    m_OwnsNativeObjects = false;
    m_Memory = (MemoryVK*)bufferDesc.memory;
    m_MappedMemoryOffset = bufferDesc.memoryOffset;

    m_Desc.size = bufferDesc.size;
    m_Desc.structureStride = bufferDesc.structureStride;

    m_Handle = (VkBuffer)bufferDesc.vkBuffer;
    m_DeviceAddress = (VkDeviceAddress)bufferDesc.deviceAddress;

    return Result::SUCCESS;
}

void BufferVK::SetHostMemory(MemoryVK& memory, uint64_t memoryOffset) {
    m_Memory = &memory;
    m_MappedMemoryOffset = memoryOffset;
}

void BufferVK::ReadDeviceAddress() {
    if (!m_Device.m_IsDeviceAddressSupported)
        return;

    VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    bufferDeviceAddressInfo.buffer = m_Handle;

    const auto& vk = m_Device.GetDispatchTable();
    m_DeviceAddress = vk.GetBufferDeviceAddress(m_Device, &bufferDeviceAddressInfo);
}

//================================================================================================================
// NRI
//================================================================================================================

void BufferVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_BUFFER, (uint64_t)m_Handle, name);
}

void BufferVK::GetMemoryInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    VkMemoryDedicatedRequirements dedicatedRequirements = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};

    VkMemoryRequirements2 requirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    requirements.pNext = &dedicatedRequirements;

    VkBufferMemoryRequirementsInfo2 info = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2};
    info.buffer = m_Handle;

    const auto& vk = m_Device.GetDispatchTable();
    vk.GetBufferMemoryRequirements2(m_Device, &info, &requirements);

    MemoryTypeUnion memoryType = {};
    memoryType.unpacked.isDedicated = dedicatedRequirements.requiresDedicatedAllocation;

    bool found = m_Device.GetMemoryType(memoryLocation, requirements.memoryRequirements.memoryTypeBits, memoryType.unpacked);
    RETURN_ON_FAILURE(&m_Device, found, ReturnVoid(), "Can't find suitable memory type");

    memoryDesc.size = requirements.memoryRequirements.size;
    memoryDesc.alignment = (uint32_t)requirements.memoryRequirements.alignment;
    memoryDesc.type = memoryType.packed;
    memoryDesc.mustBeDedicated = dedicatedRequirements.requiresDedicatedAllocation;
}

inline void* BufferVK::Map(uint64_t offset, uint64_t size) {
    CHECK(m_Memory != nullptr, "The buffer does not support memory mapping.");

    m_MappedRangeOffset = offset;
    m_MappedRangeSize = size;

    if (size == WHOLE_SIZE)
        size = m_Desc.size;

    return m_Memory->GetMappedMemory() + m_MappedMemoryOffset + offset;
}

inline void BufferVK::Unmap() {
    // TODO: flush memory range if not HOST_COHERENT
    // if (m_Memory->IsHostCoherent())
    //     m_Memory->FlushMemoryRange(m_MappedMemoryOffset + m_MappedRangeOffset, m_MappedRangeSize);
}

#include "BufferVK.hpp"