// © 2021 NVIDIA Corporation

Result CommandQueueD3D12::Create(CommandQueueType commandQueueType) {
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Priority = commandQueueType == CommandQueueType::HIGH_PRIORITY_COPY ? D3D12_COMMAND_QUEUE_PRIORITY_HIGH : D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.NodeMask = NRI_NODE_MASK;
    commandQueueDesc.Type = GetCommandListType(commandQueueType);

    HRESULT hr = m_Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_CommandQueue));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommandQueue()");

    m_CommandListType = commandQueueDesc.Type;

    return Result::SUCCESS;
}

Result CommandQueueD3D12::Create(ID3D12CommandQueue* commandQueue) {
    const D3D12_COMMAND_QUEUE_DESC& commandQueueDesc = commandQueue->GetDesc();

    m_CommandQueue = commandQueue;
    m_CommandListType = commandQueueDesc.Type;

    return Result::SUCCESS;
}

NRI_INLINE void CommandQueueD3D12::Submit(const QueueSubmitDesc& queueSubmitDesc) {
    for (uint32_t i = 0; i < queueSubmitDesc.waitFenceNum; i++) {
        const FenceSubmitDesc& fenceSubmitDesc = queueSubmitDesc.waitFences[i];
        FenceD3D12* fence = (FenceD3D12*)fenceSubmitDesc.fence;
        fence->QueueWait(*this, fenceSubmitDesc.value);
    }

    if (queueSubmitDesc.commandBufferNum) {
        Scratch<ID3D12CommandList*> commandLists = AllocateScratch(m_Device, ID3D12CommandList*, queueSubmitDesc.commandBufferNum);
        for (uint32_t j = 0; j < queueSubmitDesc.commandBufferNum; j++)
            commandLists[j] = *(CommandBufferD3D12*)queueSubmitDesc.commandBuffers[j];

        m_CommandQueue->ExecuteCommandLists(queueSubmitDesc.commandBufferNum, commandLists);
    }

    for (uint32_t i = 0; i < queueSubmitDesc.signalFenceNum; i++) {
        const FenceSubmitDesc& fenceSubmitDesc = queueSubmitDesc.signalFences[i];
        FenceD3D12* fence = (FenceD3D12*)fenceSubmitDesc.fence;
        fence->QueueSignal(*this, fenceSubmitDesc.value);
    }
}

NRI_INLINE Result CommandQueueD3D12::UploadData(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    HelperDataUpload helperDataUpload(m_Device.GetCoreInterface(), (Device&)m_Device, (CommandQueue&)*this);

    return helperDataUpload.UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

NRI_INLINE Result CommandQueueD3D12::WaitForIdle() {
    return WaitIdle(m_Device.GetCoreInterface(), (Device&)m_Device, (CommandQueue&)*this);
}
