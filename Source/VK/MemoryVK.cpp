// © 2021 NVIDIA Corporation

#include "SharedVK.h"

#include "BufferVK.h"
#include "MemoryVK.h"
#include "TextureVK.h"

using namespace nri;

MemoryVK::~MemoryVK() {
    const auto& vk = m_Device.GetDispatchTable();

    if (m_OwnsNativeObjects)
        vk.FreeMemory(m_Device, m_Handle, m_Device.GetAllocationCallbacks());
}

Result MemoryVK::Create(const MemoryType memoryType, uint64_t size) {
    m_OwnsNativeObjects = true;
    m_Type = memoryType;

    const MemoryTypeUnpack unpack = {memoryType};
    const MemoryTypeInfo& memoryTypeInfo = unpack.info;

    // Dedicated allocation occurs on memory binding
    if (memoryTypeInfo.isDedicated)
        return Result::SUCCESS;

    VkMemoryAllocateFlagsInfo flagsInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    flagsInfo.flags = (m_Device.m_IsDeviceAddressSupported ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT : 0) | VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT;
    flagsInfo.deviceMask = NRI_NODE_MASK;

    VkMemoryAllocateInfo memoryInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memoryInfo.pNext = &flagsInfo;
    memoryInfo.allocationSize = size;
    memoryInfo.memoryTypeIndex = memoryTypeInfo.memoryTypeIndex;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.AllocateMemory(m_Device, &memoryInfo, m_Device.GetAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkAllocateMemory returned %d", (int32_t)result);

    if (IsHostVisibleMemory(memoryTypeInfo.memoryLocation)) {
        result = vk.MapMemory(m_Device, m_Handle, 0, size, 0, (void**)&m_MappedMemory);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkMapMemory returned %d", (int32_t)result);
    }

    return Result::SUCCESS;
}

Result MemoryVK::Create(const MemoryVKDesc& memoryDesc) {
    m_OwnsNativeObjects = false;

    MemoryTypeUnpack unpack = {};
    const bool found = m_Device.GetMemoryTypeByIndex(memoryDesc.memoryTypeIndex, unpack.info);
    RETURN_ON_FAILURE(&m_Device, found, Result::INVALID_ARGUMENT, "Can't find memory by index");

    m_Handle = (VkDeviceMemory)memoryDesc.vkDeviceMemory;
    const MemoryTypeInfo& memoryTypeInfo = unpack.info;

    const auto& vk = m_Device.GetDispatchTable();
    if (IsHostVisibleMemory(memoryTypeInfo.memoryLocation)) {
        VkResult result = vk.MapMemory(m_Device, m_Handle, 0, memoryDesc.size, 0, (void**)&m_MappedMemory);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkMapMemory returned %d", (int32_t)result);
    }

    return Result::SUCCESS;
}

Result MemoryVK::CreateDedicated(BufferVK& buffer) {
    m_OwnsNativeObjects = true;

    RETURN_ON_FAILURE(&m_Device, m_Type != std::numeric_limits<MemoryType>::max(), Result::FAILURE, "Can't allocate a dedicated memory: memory type is invalid.");

    const MemoryTypeUnpack unpack = {m_Type};
    const MemoryTypeInfo& memoryTypeInfo = unpack.info;

    RETURN_ON_FAILURE(&m_Device, memoryTypeInfo.isDedicated == 1, Result::FAILURE, "Can't allocate a dedicated memory: memory type is not dedicated.");

    MemoryDesc memoryDesc = {};
    buffer.GetMemoryInfo(memoryTypeInfo.memoryLocation, memoryDesc);

    VkMemoryAllocateFlagsInfo flagsInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    flagsInfo.flags = (m_Device.m_IsDeviceAddressSupported ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT : 0) | VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT;
    flagsInfo.deviceMask = NRI_NODE_MASK;

    VkMemoryDedicatedAllocateInfo dedicatedAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    dedicatedAllocateInfo.pNext = &flagsInfo;
    dedicatedAllocateInfo.buffer = buffer.GetHandle();

    VkMemoryAllocateInfo memoryInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memoryInfo.pNext = &dedicatedAllocateInfo;
    memoryInfo.allocationSize = memoryDesc.size;
    memoryInfo.memoryTypeIndex = memoryTypeInfo.memoryTypeIndex;

    // No need to allocate more than one instance of host memory
    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.AllocateMemory(m_Device, &memoryInfo, m_Device.GetAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkAllocateMemory returned %d", (int32_t)result);

    if (IsHostVisibleMemory(memoryTypeInfo.memoryLocation)) {
        result = vk.MapMemory(m_Device, m_Handle, 0, memoryDesc.size, 0, (void**)&m_MappedMemory);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkMapMemory returned %d", (int32_t)result);
    }

    return Result::SUCCESS;
}

Result MemoryVK::CreateDedicated(TextureVK& texture) {
    m_OwnsNativeObjects = true;

    RETURN_ON_FAILURE(&m_Device, m_Type != std::numeric_limits<MemoryType>::max(), Result::FAILURE, "Can't allocate a dedicated memory: invalid memory type.");

    const MemoryTypeUnpack unpack = {m_Type};
    const MemoryTypeInfo& memoryTypeInfo = unpack.info;

    RETURN_ON_FAILURE(&m_Device, memoryTypeInfo.isDedicated == 1, Result::FAILURE, "Can't allocate a dedicated memory: the memory type is not dedicated.");

    MemoryDesc memoryDesc = {};
    texture.GetMemoryInfo(memoryTypeInfo.memoryLocation, memoryDesc);

    VkMemoryAllocateFlagsInfo flagsInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    flagsInfo.flags = (m_Device.m_IsDeviceAddressSupported ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT : 0) | VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT;
    flagsInfo.deviceMask = NRI_NODE_MASK;

    VkMemoryDedicatedAllocateInfo dedicatedAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    dedicatedAllocateInfo.pNext = &flagsInfo;
    dedicatedAllocateInfo.image = texture.GetHandle();

    VkMemoryAllocateInfo memoryInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memoryInfo.pNext = &dedicatedAllocateInfo;
    memoryInfo.allocationSize = memoryDesc.size;
    memoryInfo.memoryTypeIndex = memoryTypeInfo.memoryTypeIndex;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.AllocateMemory(m_Device, &memoryInfo, m_Device.GetAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkAllocateMemory returned %d", (int32_t)result);

    if (IsHostVisibleMemory(memoryTypeInfo.memoryLocation)) {
        result = vk.MapMemory(m_Device, m_Handle, 0, memoryDesc.size, 0, (void**)&m_MappedMemory);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkMapMemory returned %d", (int32_t)result);
    }

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

void MemoryVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)m_Handle, name);
}
