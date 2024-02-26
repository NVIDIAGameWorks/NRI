// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static void NRI_CALL SetCommandQueueDebugName(CommandQueue& commandQueue, const char* name) {
    ((CommandQueueD3D12&)commandQueue).SetDebugName(name);
}

static void NRI_CALL QueueSubmit(CommandQueue& commandQueue, const QueueSubmitDesc& queueSubmitDesc) {
    ((CommandQueueD3D12&)commandQueue).Submit(queueSubmitDesc);
}

#pragma endregion

#pragma region[  Helper  ]

static Result NRI_CALL UploadData(CommandQueue& commandQueue, const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs,
    uint32_t bufferUploadDescNum) {
    return ((CommandQueueD3D12&)commandQueue).UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

static Result NRI_CALL WaitForIdle(CommandQueue& commandQueue) {
    if (!(&commandQueue))
        return Result::SUCCESS;

    return ((CommandQueueD3D12&)commandQueue).WaitForIdle();
}

#pragma endregion

Define_Core_CommandQueue_PartiallyFillFunctionTable(D3D12) Define_Helper_CommandQueue_PartiallyFillFunctionTable(D3D12)
