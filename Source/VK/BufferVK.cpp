// © 2021 NVIDIA Corporation

#include "SharedVK.h"

#include "BufferVK.h"
#include "CommandQueueVK.h"
#include "MemoryVK.h"

using namespace nri;

BufferVK::~BufferVK() {
    const auto& vk = m_Device.GetDispatchTable();

    if (m_OwnsNativeObjects)
        vk.DestroyBuffer(m_Device, m_Handle, m_Device.GetAllocationCallbacks());
}

Result BufferVK::Create(const BufferDesc& bufferDesc) {
    m_OwnsNativeObjects = true;
    m_Desc = bufferDesc;

    VkBufferCreateInfo info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    m_Device.FillCreateInfo(bufferDesc, info);

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