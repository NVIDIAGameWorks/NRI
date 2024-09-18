// © 2021 NVIDIA Corporation

Result CreateCommandBuffer(DeviceD3D11& device, ID3D11DeviceContext* precreatedContext, CommandBuffer*& commandBuffer) {
    bool isImmediate = false;
    if (precreatedContext)
        isImmediate = precreatedContext->GetType() == D3D11_DEVICE_CONTEXT_IMMEDIATE;
    else
        isImmediate = device.IsDeferredContextEmulated();

    void* impl;
    if (isImmediate)
        impl = Allocate<CommandBufferEmuD3D11>(device.GetStdAllocator(), device);
    else
        impl = Allocate<CommandBufferD3D11>(device.GetStdAllocator(), device);

    const Result result = ((CommandBufferHelper*)impl)->Create(precreatedContext);

    if (result == Result::SUCCESS) {
        commandBuffer = (CommandBuffer*)impl;
        return Result::SUCCESS;
    }

    if (isImmediate)
        Destroy(device.GetStdAllocator(), (CommandBufferEmuD3D11*)impl);
    else
        Destroy(device.GetStdAllocator(), (CommandBufferD3D11*)impl);

    return result;
}

NRI_INLINE Result CommandAllocatorD3D11::CreateCommandBuffer(CommandBuffer*& commandBuffer) {
    return ::CreateCommandBuffer(m_Device, nullptr, commandBuffer);
}
