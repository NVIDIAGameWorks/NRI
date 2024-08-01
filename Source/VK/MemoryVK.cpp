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

Result MemoryVK::Create(const MemoryVKDesc& memoryDesc) {
    m_OwnsNativeObjects = false;

    MemoryTypeUnion memoryType = {};

    bool found = m_Device.GetMemoryTypeByIndex(memoryDesc.memoryTypeIndex, memoryType.unpacked);
    RETURN_ON_FAILURE(&m_Device, found, Result::INVALID_ARGUMENT, "Can't find memory by index");

    m_Handle = (VkDeviceMemory)memoryDesc.vkDeviceMemory;
    m_MappedMemory = (uint8_t*)memoryDesc.vkMappedMemory;
    m_Type = memoryType.packed;

    const auto& vk = m_Device.GetDispatchTable();
    if (!m_MappedMemory && IsHostVisibleMemory(memoryType.unpacked.location)) {
        VkResult result = vk.MapMemory(m_Device, m_Handle, 0, memoryDesc.size, 0, (void**)&m_MappedMemory);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkMapMemory returned %d", (int32_t)result);
    }

    return Result::SUCCESS;
}

Result MemoryVK::Create(MemoryType type, uint64_t size) {
    m_OwnsNativeObjects = true;
    m_Type = type;

    MemoryTypeUnion memoryType = {type};
    if (memoryType.unpacked.isDedicated)
        return Result::SUCCESS; // dedicated allocation occurs on memory binding

    VkMemoryAllocateFlagsInfo flagsInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    flagsInfo.flags = (m_Device.m_IsDeviceAddressSupported ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT : 0) | VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT;
    flagsInfo.deviceMask = NRI_NODE_MASK;

    VkMemoryAllocateInfo memoryInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memoryInfo.pNext = &flagsInfo;
    memoryInfo.allocationSize = size;
    memoryInfo.memoryTypeIndex = memoryType.unpacked.index;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.AllocateMemory(m_Device, &memoryInfo, m_Device.GetAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkAllocateMemory returned %d", (int32_t)result);

    if (IsHostVisibleMemory(memoryType.unpacked.location)) {
        result = vk.MapMemory(m_Device, m_Handle, 0, size, 0, (void**)&m_MappedMemory);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkMapMemory returned %d", (int32_t)result);
    }

    return Result::SUCCESS;
}

Result MemoryVK::CreateDedicated(const BufferVK& buffer) {
    m_OwnsNativeObjects = true;

    MemoryTypeUnion memoryType = {m_Type};
    CHECK(m_Type != std::numeric_limits<MemoryType>::max() && memoryType.unpacked.isDedicated, "Shouldn't be there");

    MemoryDesc memoryDesc = {};
    m_Device.GetMemoryDesc(buffer.GetDesc(), memoryType.unpacked.location, memoryDesc);

    VkMemoryAllocateFlagsInfo flagsInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    flagsInfo.flags = (m_Device.m_IsDeviceAddressSupported ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT : 0) | VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT;
    flagsInfo.deviceMask = NRI_NODE_MASK;

    VkMemoryDedicatedAllocateInfo dedicatedAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    dedicatedAllocateInfo.pNext = &flagsInfo;
    dedicatedAllocateInfo.buffer = buffer.GetHandle();

    VkMemoryAllocateInfo memoryInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memoryInfo.pNext = &dedicatedAllocateInfo;
    memoryInfo.allocationSize = memoryDesc.size;
    memoryInfo.memoryTypeIndex = memoryType.unpacked.index;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.AllocateMemory(m_Device, &memoryInfo, m_Device.GetAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkAllocateMemory returned %d", (int32_t)result);

    if (IsHostVisibleMemory(memoryType.unpacked.location)) {
        result = vk.MapMemory(m_Device, m_Handle, 0, memoryDesc.size, 0, (void**)&m_MappedMemory);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkMapMemory returned %d", (int32_t)result);
    }

    return Result::SUCCESS;
}

Result MemoryVK::CreateDedicated(const TextureVK& texture) {
    m_OwnsNativeObjects = true;

    const MemoryTypeUnion memoryType = {m_Type};
    CHECK(m_Type != std::numeric_limits<MemoryType>::max() && memoryType.unpacked.isDedicated, "Shouldn't be there");

    MemoryDesc memoryDesc = {};
    m_Device.GetMemoryDesc(texture.GetDesc(), memoryType.unpacked.location, memoryDesc);

    VkMemoryAllocateFlagsInfo flagsInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    flagsInfo.flags = (m_Device.m_IsDeviceAddressSupported ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT : 0) | VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT;
    flagsInfo.deviceMask = NRI_NODE_MASK;

    VkMemoryDedicatedAllocateInfo dedicatedAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    dedicatedAllocateInfo.pNext = &flagsInfo;
    dedicatedAllocateInfo.image = texture.GetHandle();

    VkMemoryAllocateInfo memoryInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memoryInfo.pNext = &dedicatedAllocateInfo;
    memoryInfo.allocationSize = memoryDesc.size;
    memoryInfo.memoryTypeIndex = memoryType.unpacked.index;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.AllocateMemory(m_Device, &memoryInfo, m_Device.GetAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkAllocateMemory returned %d", (int32_t)result);

    if (IsHostVisibleMemory(memoryType.unpacked.location)) {
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
