// © 2021 NVIDIA Corporation

NRI_INLINE void CommandAllocatorVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetCommandAllocatorDebugName(*GetImpl(), name);
}

NRI_INLINE Result CommandAllocatorVal::CreateCommandBuffer(CommandBuffer*& commandBuffer) {
    CommandBuffer* commandBufferImpl;
    const Result result = GetCoreInterface().CreateCommandBuffer(*GetImpl(), commandBufferImpl);

    if (result == Result::SUCCESS)
        commandBuffer = (CommandBuffer*)Allocate<CommandBufferVal>(m_Device.GetStdAllocator(), m_Device, commandBufferImpl, false);

    return result;
}

NRI_INLINE void CommandAllocatorVal::Reset() {
    GetCoreInterface().ResetCommandAllocator(*GetImpl());
}
