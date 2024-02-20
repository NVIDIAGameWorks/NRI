// © 2021 NVIDIA Corporation

#include "SharedVK.h"
#include "MemoryVK.h"
#include "BufferVK.h"
#include "TextureVK.h"

using namespace nri;

MemoryVK::~MemoryVK()
{
    const auto& vk = m_Device.GetDispatchTable();

    if (!m_OwnsNativeObjects)
        return;

    for (uint32_t i = 0; i < GetCountOf(m_Handles); i++)
    {
        if (m_Handles[i] != VK_NULL_HANDLE)
            vk.FreeMemory(m_Device, m_Handles[i], m_Device.GetAllocationCallbacks());
    }
}

Result MemoryVK::Create(uint32_t nodeMask, const MemoryType memoryType, uint64_t size)
{
    m_OwnsNativeObjects = true;
    m_Type = memoryType;

    const MemoryTypeUnpack unpack = { memoryType };
    const MemoryTypeInfo& memoryTypeInfo = unpack.info;

    // Dedicated allocation occurs on memory binding
    if (memoryTypeInfo.isDedicated == 1)
        return Result::SUCCESS;

    VkMemoryAllocateFlagsInfo flagsInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };
    flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT;

    if (m_Device.m_IsDeviceAddressSupported)
        flagsInfo.flags |= VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    VkMemoryAllocateInfo memoryInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    memoryInfo.pNext = &flagsInfo;
    memoryInfo.allocationSize = size;
    memoryInfo.memoryTypeIndex = memoryTypeInfo.memoryTypeIndex;

    nodeMask = GetNodeMask(nodeMask);

    // No need to allocate more than one instance of host memory
    if (IsHostMemory(memoryTypeInfo.memoryLocation))
    {
        nodeMask = 0x1;
        flagsInfo.flags &= ~VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT;
    }

    const auto& vk = m_Device.GetDispatchTable();

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if ((1 << i) & nodeMask)
        {
            flagsInfo.deviceMask = 1 << i;

            VkResult result = vk.AllocateMemory(m_Device, &memoryInfo, m_Device.GetAllocationCallbacks(), &m_Handles[i]);

            RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result),
                "Can't allocate a memory: vkAllocateMemory returned %d.", (int32_t)result);

            if (IsHostVisibleMemory(memoryTypeInfo.memoryLocation))
            {
                result = vk.MapMemory(m_Device, m_Handles[i], 0, size, 0, (void**)&m_MappedMemory[i]);

                RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result),
                    "Can't map the allocated memory: vkMapMemory returned %d.", (int32_t)result);
            }
        }
    }

    return Result::SUCCESS;
}

Result MemoryVK::Create(const MemoryVKDesc& memoryDesc)
{
    m_OwnsNativeObjects = false;

    MemoryTypeUnpack unpack = {};
    const bool found = m_Device.GetMemoryTypeByIndex(memoryDesc.memoryTypeIndex, unpack.info);
    RETURN_ON_FAILURE(&m_Device, found, Result::INVALID_ARGUMENT, "Can't find memory by index");

    const VkDeviceMemory handle = (VkDeviceMemory)memoryDesc.vkDeviceMemory;
    const uint32_t nodeMask = GetNodeMask(memoryDesc.nodeMask);

    const MemoryTypeInfo& memoryTypeInfo = unpack.info;

    const auto& vk = m_Device.GetDispatchTable();

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if ((1 << i) & nodeMask)
        {
            m_Handles[i] = handle;

            if (IsHostVisibleMemory(memoryTypeInfo.memoryLocation))
            {
                const VkResult result = vk.MapMemory(m_Device, m_Handles[i], 0, memoryDesc.size, 0, (void**)&m_MappedMemory[i]);

                RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result),
                    "Can't map the memory: vkMapMemory returned %d.", (int32_t)result);
            }
        }
    }

    return Result::SUCCESS;
}

Result MemoryVK::CreateDedicated(BufferVK& buffer, uint32_t nodeMask)
{
    m_OwnsNativeObjects = true;

    RETURN_ON_FAILURE(&m_Device, m_Type != std::numeric_limits<MemoryType>::max(), Result::FAILURE,
        "Can't allocate a dedicated memory: memory type is invalid.");

    const MemoryTypeUnpack unpack = { m_Type };
    const MemoryTypeInfo& memoryTypeInfo = unpack.info;

    RETURN_ON_FAILURE(&m_Device, memoryTypeInfo.isDedicated == 1, Result::FAILURE,
        "Can't allocate a dedicated memory: memory type is not dedicated.");

    MemoryDesc memoryDesc = {};
    buffer.GetMemoryInfo(memoryTypeInfo.memoryLocation, memoryDesc);

    VkMemoryAllocateFlagsInfo flagsInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };
    flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    VkMemoryDedicatedAllocateInfo dedicatedAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO };
    dedicatedAllocateInfo.pNext = &flagsInfo;

    VkMemoryAllocateInfo memoryInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    memoryInfo.pNext = &dedicatedAllocateInfo;
    memoryInfo.allocationSize = memoryDesc.size;
    memoryInfo.memoryTypeIndex = memoryTypeInfo.memoryTypeIndex;

    // No need to allocate more than one instance of host memory
    if (IsHostMemory(memoryTypeInfo.memoryLocation))
    {
        nodeMask = 0x1;
        flagsInfo.flags &= ~VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT;
    }

    const auto& vk = m_Device.GetDispatchTable();

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if ((1 << i) & nodeMask)
        {
            dedicatedAllocateInfo.buffer = buffer.GetHandle(i);
            flagsInfo.deviceMask = 1 << i;

            VkResult result = vk.AllocateMemory(m_Device, &memoryInfo, m_Device.GetAllocationCallbacks(), &m_Handles[i]);

            RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result),
                "Can't allocate a dedicated memory: vkAllocateMemory returned %d.", (int32_t)result);

            if (IsHostVisibleMemory(memoryTypeInfo.memoryLocation))
            {
                result = vk.MapMemory(m_Device, m_Handles[i], 0, memoryDesc.size, 0, (void**)&m_MappedMemory[i]);

                RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result),
                    "Can't map the allocated memory: vkMapMemory returned %d.", (int32_t)result);
            }
        }
    }

    return Result::SUCCESS;
}

Result MemoryVK::CreateDedicated(TextureVK& texture, uint32_t nodeMask)
{
    m_OwnsNativeObjects = true;

    RETURN_ON_FAILURE(&m_Device, m_Type != std::numeric_limits<MemoryType>::max(), Result::FAILURE,
        "Can't allocate a dedicated memory: invalid memory type.");

    const MemoryTypeUnpack unpack = { m_Type };
    const MemoryTypeInfo& memoryTypeInfo = unpack.info;

    RETURN_ON_FAILURE(&m_Device, memoryTypeInfo.isDedicated == 1, Result::FAILURE,
        "Can't allocate a dedicated memory: the memory type is not dedicated.");

    MemoryDesc memoryDesc = {};
    texture.GetMemoryInfo(memoryTypeInfo.memoryLocation, memoryDesc);

    VkMemoryAllocateFlagsInfo flagsInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };
    flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT;

    VkMemoryDedicatedAllocateInfo dedicatedAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO };
    dedicatedAllocateInfo.pNext = &flagsInfo;

    VkMemoryAllocateInfo memoryInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    memoryInfo.pNext = &dedicatedAllocateInfo;
    memoryInfo.allocationSize = memoryDesc.size;
    memoryInfo.memoryTypeIndex = memoryTypeInfo.memoryTypeIndex;

    // No need to allocate more than one instance of host memory
    if (IsHostMemory(memoryTypeInfo.memoryLocation))
    {
        nodeMask = 0x1;
        dedicatedAllocateInfo.pNext = nullptr;
    }

    const auto& vk = m_Device.GetDispatchTable();

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if ((1 << i) & nodeMask)
        {
            dedicatedAllocateInfo.image = texture.GetHandle(i);
            flagsInfo.deviceMask = 1 << i;

            VkResult result = vk.AllocateMemory(m_Device, &memoryInfo, m_Device.GetAllocationCallbacks(), &m_Handles[i]);

            RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result),
                "Can't allocate a dedicated memory: vkAllocateMemory returned %d.", (int32_t)result);

            if (IsHostVisibleMemory(memoryTypeInfo.memoryLocation))
            {
                result = vk.MapMemory(m_Device, m_Handles[i], 0, memoryDesc.size, 0, (void**)&m_MappedMemory[i]);

                RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result),
                    "Can't map the allocated memory: vkMapMemory returned %d.", (int32_t)result);
            }
        }
    }

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

void MemoryVK::SetDebugName(const char* name)
{
    std::array<uint64_t, PHYSICAL_DEVICE_GROUP_MAX_SIZE> handles;
    for (size_t i = 0; i < handles.size(); i++)
        handles[i] = (uint64_t)m_Handles[i];

    m_Device.SetDebugNameToDeviceGroupObject(VK_OBJECT_TYPE_DEVICE_MEMORY, handles.data(), name);
}
