// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static void NRI_CALL SetCommandQueueDebugName(CommandQueue& commandQueue, const char* name) {
    ((CommandQueueVK&)commandQueue).SetDebugName(name);
}

static void NRI_CALL QueueSubmit(CommandQueue& commandQueue, const QueueSubmitDesc& workSubmissionDesc) {
    ((CommandQueueVK&)commandQueue).Submit(workSubmissionDesc);
}

#pragma endregion

#pragma region[  Helper  ]

static Result NRI_CALL UploadData(CommandQueue& commandQueue, const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs,
    uint32_t bufferUploadDescNum) {
    return ((CommandQueueVK&)commandQueue).UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

static Result NRI_CALL WaitForIdle(CommandQueue& commandQueue) {
    if (!(&commandQueue))
        return Result::SUCCESS;

    return ((CommandQueueVK&)commandQueue).WaitForIdle();
}

#pragma endregion

Define_Core_CommandQueue_PartiallyFillFunctionTable(VK) Define_Helper_CommandQueue_PartiallyFillFunctionTable(VK)
