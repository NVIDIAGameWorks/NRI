// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static void NRI_CALL SetBufferDebugName(Buffer& buffer, const char* name) {
    ((BufferVK&)buffer).SetDebugName(name);
}

static uint64_t NRI_CALL GetBufferNativeObject(const Buffer& buffer) {
    if (!(&buffer))
        return 0;

    return uint64_t(((BufferVK&)buffer).GetHandle());
}

static void NRI_CALL GetBufferMemoryInfo(const Buffer& buffer, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((BufferVK&)buffer).GetMemoryInfo(memoryLocation, memoryDesc);
}

static void* NRI_CALL MapBuffer(Buffer& buffer, uint64_t offset, uint64_t size) {
    return ((BufferVK&)buffer).Map(offset, size);
}

static void NRI_CALL UnmapBuffer(Buffer& buffer) {
    ((BufferVK&)buffer).Unmap();
}

#pragma endregion

Define_Core_Buffer_PartiallyFillFunctionTable(VK);
