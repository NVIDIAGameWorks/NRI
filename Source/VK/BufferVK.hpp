// © 2021 NVIDIA Corporation

BufferVK::~BufferVK() {
    if (m_OwnsNativeObjects) {
        const auto& vk = m_Device.GetDispatchTable();

        if (m_VmaAllocation)
            DestroyVma();
        else
            vk.DestroyBuffer(m_Device, m_Handle, m_Device.GetAllocationCallbacks());
    }
}

Result BufferVK::Create(const BufferDesc& bufferDesc) {
    m_Desc = bufferDesc;

    VkBufferCreateInfo info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    m_Device.FillCreateInfo(bufferDesc, info);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateBuffer(m_Device, &info, m_Device.GetAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateBuffer returned %d", (int32_t)result);

    return Result::SUCCESS;
}

Result BufferVK::Create(const BufferVKDesc& bufferDesc) {
    if (!bufferDesc.vkBuffer)
        return Result::INVALID_ARGUMENT;

    m_OwnsNativeObjects = false;
    m_Handle = (VkBuffer)bufferDesc.vkBuffer;
    m_MappedMemory = bufferDesc.mappedMemory;
    m_NonCoherentDeviceMemory = (VkDeviceMemory)bufferDesc.vkDeviceMemory;
    m_DeviceAddress = (VkDeviceAddress)bufferDesc.deviceAddress;

    m_Desc.size = bufferDesc.size;
    m_Desc.structureStride = bufferDesc.structureStride;

    return Result::SUCCESS;
}

void BufferVK::FinishMemoryBinding(MemoryVK& memory, uint64_t memoryOffset) {
    CHECK(m_OwnsNativeObjects, "Not for wrapped objects");

    // Mapped memory
    MemoryTypeInfo memoryTypeInfo = Unpack(memory.GetType());
    if (IsHostVisibleMemory(memoryTypeInfo.location)) {
        m_MappedMemory = memory.GetMappedMemory();
        m_MappedMemoryOffset = memoryOffset;

        if (!m_Device.IsHostCoherentMemory(memoryTypeInfo.index))
            m_NonCoherentDeviceMemory = memory.GetHandle();
    }

    // Device address
    if (m_Device.m_IsSupported.deviceAddress) {
        VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        bufferDeviceAddressInfo.buffer = m_Handle;

        const auto& vk = m_Device.GetDispatchTable();
        m_DeviceAddress = vk.GetBufferDeviceAddress(m_Device, &bufferDeviceAddressInfo);
    }
}

NRI_INLINE void BufferVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_BUFFER, (uint64_t)m_Handle, name);
}

NRI_INLINE void* BufferVK::Map(uint64_t offset, uint64_t size) {
    CHECK(m_MappedMemory, "The buffer does not support memory mapping");

    if (size == WHOLE_SIZE)
        size = m_Desc.size;

    m_MappedMemoryRangeSize = size;
    m_MappedMemoryRangeOffset = offset;

    offset += m_MappedMemoryOffset;

    return m_MappedMemory + offset;
}

NRI_INLINE void BufferVK::Unmap() {
    if (m_NonCoherentDeviceMemory) {
        VkMappedMemoryRange memoryRange = {VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
        memoryRange.memory = m_NonCoherentDeviceMemory;
        memoryRange.offset = m_MappedMemoryOffset + m_MappedMemoryRangeOffset;
        memoryRange.size = m_MappedMemoryRangeSize;

        const auto& vk = m_Device.GetDispatchTable();
        VkResult result = vk.FlushMappedMemoryRanges(m_Device, 1, &memoryRange);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, ReturnVoid(), "vkFlushMappedMemoryRanges returned %d", (int32_t)result);
    }
}
