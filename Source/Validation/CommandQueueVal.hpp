// © 2021 NVIDIA Corporation

static bool ValidateTextureUploadDesc(DeviceVal& device, uint32_t i, const TextureUploadDesc& textureUploadDesc) {
    if (!textureUploadDesc.subresources) {
        REPORT_WARNING(&device, "the number of subresources in 'textureUploadDescs[%u]' is 0 (nothing to upload)", i);
        return true;
    }

    const TextureVal& textureVal = *(TextureVal*)textureUploadDesc.texture;
    const TextureDesc& textureDesc = textureVal.GetDesc();

    RETURN_ON_FAILURE(&device, textureUploadDesc.texture != nullptr, false, "'textureUploadDescs[%u].texture' is NULL", i);
    RETURN_ON_FAILURE(&device, textureUploadDesc.after.layout < Layout::MAX_NUM, false, "'textureUploadDescs[%u].nextLayout' is invalid", i);
    RETURN_ON_FAILURE(&device, textureVal.IsBoundToMemory(), false, "'textureUploadDescs[%u].texture' is not bound to memory", i);

    uint32_t subresourceNum = textureDesc.layerNum * textureDesc.mipNum;
    for (uint32_t j = 0; j < subresourceNum; j++) {
        const TextureSubresourceUploadDesc& subresource = textureUploadDesc.subresources[j];

        if (subresource.sliceNum == 0) {
            REPORT_WARNING(&device, "No data to upload: the number of subresources in 'textureUploadDescs[%u].subresources[%u].sliceNum' is 0", i, j);
            continue;
        }

        RETURN_ON_FAILURE(&device, subresource.slices != nullptr, false, "'textureUploadDescs[%u].subresources[%u].slices' is invalid", i, j);
        RETURN_ON_FAILURE(&device, subresource.rowPitch != 0, false, "'textureUploadDescs[%u].subresources[%u].rowPitch' is 0", i, j);
        RETURN_ON_FAILURE(&device, subresource.slicePitch != 0, false, "'textureUploadDescs[%u].subresources[%u].slicePitch' is 0", i, j);
    }

    return true;
}

static bool ValidateBufferUploadDesc(DeviceVal& device, uint32_t i, const BufferUploadDesc& bufferUploadDesc) {
    if (bufferUploadDesc.dataSize == 0) {
        REPORT_WARNING(&device, "'bufferUploadDescs[%u].dataSize' is 0 (nothing to upload)", i);
        return true;
    }

    const BufferVal& bufferVal = *(BufferVal*)bufferUploadDesc.buffer;
    const uint64_t rangeEnd = bufferUploadDesc.bufferOffset + bufferUploadDesc.dataSize;

    RETURN_ON_FAILURE(&device, bufferUploadDesc.buffer != nullptr, false, "'bufferUploadDescs[%u].buffer' is invalid", i);
    RETURN_ON_FAILURE(&device, bufferUploadDesc.data != nullptr, false, "'bufferUploadDescs[%u].data' is invalid", i);
    RETURN_ON_FAILURE(&device, bufferVal.IsBoundToMemory(), false, "'bufferUploadDescs[%u].buffer' is not bound to memory", i);
    RETURN_ON_FAILURE(&device, rangeEnd <= bufferVal.GetDesc().size, false, "'bufferUploadDescs[%u].bufferOffset + bufferUploadDescs[%u].dataSize' is out of bounds", i, i);

    return true;
}

NRI_INLINE void CommandQueueVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetCommandQueueDebugName(*GetImpl(), name);
}

NRI_INLINE void CommandQueueVal::Submit(const QueueSubmitDesc& queueSubmitDesc, const SwapChain* swapChain) {
    ProcessValidationCommands((const CommandBufferVal* const*)queueSubmitDesc.commandBuffers, queueSubmitDesc.commandBufferNum);

    auto queueSubmitDescImpl = queueSubmitDesc;

    Scratch<FenceSubmitDesc> waitFences = AllocateScratch(m_Device, FenceSubmitDesc, queueSubmitDesc.waitFenceNum);
    for (uint32_t i = 0; i < queueSubmitDesc.waitFenceNum; i++) {
        waitFences[i] = queueSubmitDesc.waitFences[i];
        waitFences[i].fence = NRI_GET_IMPL(Fence, waitFences[i].fence);
    }
    queueSubmitDescImpl.waitFences = waitFences;

    Scratch<CommandBuffer*> commandBuffers = AllocateScratch(m_Device, CommandBuffer*, queueSubmitDesc.commandBufferNum);
    for (uint32_t i = 0; i < queueSubmitDesc.commandBufferNum; i++)
        commandBuffers[i] = NRI_GET_IMPL(CommandBuffer, queueSubmitDesc.commandBuffers[i]);
    queueSubmitDescImpl.commandBuffers = commandBuffers;

    Scratch<FenceSubmitDesc> signalFences = AllocateScratch(m_Device, FenceSubmitDesc, queueSubmitDesc.signalFenceNum);
    for (uint32_t i = 0; i < queueSubmitDesc.signalFenceNum; i++) {
        signalFences[i] = queueSubmitDesc.signalFences[i];
        signalFences[i].fence = NRI_GET_IMPL(Fence, signalFences[i].fence);
    }
    queueSubmitDescImpl.signalFences = signalFences;

    if (swapChain) {
        SwapChain* swapChainImpl = NRI_GET_IMPL(SwapChain, swapChain);
        m_Device.GetLowLatencyInterface().QueueSubmitTrackable(*GetImpl(), queueSubmitDescImpl, *swapChainImpl);
    } else
        GetCoreInterface().QueueSubmit(*GetImpl(), queueSubmitDescImpl);
}

NRI_INLINE Result CommandQueueVal::UploadData(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    RETURN_ON_FAILURE(&m_Device, textureUploadDescNum == 0 || textureUploadDescs != nullptr, Result::INVALID_ARGUMENT, "'textureUploadDescs' is NULL");
    RETURN_ON_FAILURE(&m_Device, bufferUploadDescNum == 0 || bufferUploadDescs != nullptr, Result::INVALID_ARGUMENT, "'bufferUploadDescs' is NULL");

    Scratch<TextureUploadDesc> textureUploadDescsImpl = AllocateScratch(m_Device, TextureUploadDesc, textureUploadDescNum);
    for (uint32_t i = 0; i < textureUploadDescNum; i++) {
        if (!ValidateTextureUploadDesc(m_Device, i, textureUploadDescs[i]))
            return Result::INVALID_ARGUMENT;

        const TextureVal* textureVal = (TextureVal*)textureUploadDescs[i].texture;

        textureUploadDescsImpl[i] = textureUploadDescs[i];
        textureUploadDescsImpl[i].texture = textureVal->GetImpl();
    }

    Scratch<BufferUploadDesc> bufferUploadDescsImpl = AllocateScratch(m_Device, BufferUploadDesc, bufferUploadDescNum);
    for (uint32_t i = 0; i < bufferUploadDescNum; i++) {
        if (!ValidateBufferUploadDesc(m_Device, i, bufferUploadDescs[i]))
            return Result::INVALID_ARGUMENT;

        const BufferVal* bufferVal = (BufferVal*)bufferUploadDescs[i].buffer;

        bufferUploadDescsImpl[i] = bufferUploadDescs[i];
        bufferUploadDescsImpl[i].buffer = bufferVal->GetImpl();
    }

    return GetHelperInterface().UploadData(*GetImpl(), textureUploadDescsImpl, textureUploadDescNum, bufferUploadDescsImpl, bufferUploadDescNum);
}

NRI_INLINE Result CommandQueueVal::WaitForIdle() {
    return GetHelperInterface().WaitForIdle(*GetImpl());
}

template <typename Command>
const Command* ReadCommand(const uint8_t*& begin, const uint8_t* end) {
    if (begin + sizeof(Command) <= end) {
        const Command* command = (const Command*)begin;
        begin += sizeof(Command);
        return command;
    }
    return nullptr;
}

void CommandQueueVal::ProcessValidationCommandBeginQuery(const uint8_t*& begin, const uint8_t* end) {
    const ValidationCommandUseQuery* command = ReadCommand<ValidationCommandUseQuery>(begin, end);
    CHECK(command != nullptr, "can't parse command");
    CHECK(command->queryPool != nullptr, "query pool is invalid");

    QueryPoolVal& queryPool = *(QueryPoolVal*)command->queryPool;
    const bool used = queryPool.SetQueryState(command->queryPoolOffset, true);

    if (used)
        REPORT_ERROR(&m_Device, "QueryPool='%s' (offset=%u) must be reset before use", queryPool.GetDebugName(), command->queryPoolOffset);
}

void CommandQueueVal::ProcessValidationCommandEndQuery(const uint8_t*& begin, const uint8_t* end) {
    const ValidationCommandUseQuery* command = ReadCommand<ValidationCommandUseQuery>(begin, end);
    CHECK(command != nullptr, "can't parse command");
    CHECK(command->queryPool != nullptr, "query pool is invalid");

    QueryPoolVal& queryPool = *(QueryPoolVal*)command->queryPool;
    const bool used = queryPool.SetQueryState(command->queryPoolOffset, true);

    if (queryPool.GetQueryType() == QueryType::TIMESTAMP) {
        if (used)
            REPORT_ERROR(&m_Device, "QueryPool='%s' (offset=%u) must be reset before use", queryPool.GetDebugName(), command->queryPoolOffset);
    } else {
        if (!used)
            REPORT_ERROR(&m_Device, "QueryPool='%s' (offset=%u) is not in active state", queryPool.GetDebugName(), command->queryPoolOffset);
    }
}

void CommandQueueVal::ProcessValidationCommandResetQuery(const uint8_t*& begin, const uint8_t* end) {
    const ValidationCommandResetQuery* command = ReadCommand<ValidationCommandResetQuery>(begin, end);
    CHECK(command != nullptr, "can't parse command");
    CHECK(command->queryPool != nullptr, "query pool is invalid");

    QueryPoolVal& queryPool = *(QueryPoolVal*)command->queryPool;
    queryPool.ResetQueries(command->queryPoolOffset, command->queryNum);
}

void CommandQueueVal::ProcessValidationCommands(const CommandBufferVal* const* commandBuffers, uint32_t commandBufferNum) {
    ExclusiveScope lockScope(m_Device.GetLock());

    using ProcessValidationCommandMethod = void (CommandQueueVal::*)(const uint8_t*& begin, const uint8_t* end);

    constexpr ProcessValidationCommandMethod table[] = {
        &CommandQueueVal::ProcessValidationCommandBeginQuery, // ValidationCommandType::BEGIN_QUERY
        &CommandQueueVal::ProcessValidationCommandEndQuery,   // ValidationCommandType::END_QUERY
        &CommandQueueVal::ProcessValidationCommandResetQuery  // ValidationCommandType::RESET_QUERY
    };

    for (size_t i = 0; i < commandBufferNum; i++) {
        const Vector<uint8_t>& buffer = commandBuffers[i]->GetValidationCommands();
        const uint8_t* begin = buffer.data();
        const uint8_t* end = buffer.data() + buffer.size();

        while (begin != end) {
            const ValidationCommandType type = *(const ValidationCommandType*)begin;

            if (type == ValidationCommandType::NONE || type >= ValidationCommandType::MAX_NUM) {
                REPORT_ERROR(&m_Device, "Invalid validation command: %u", (uint32_t)type);
                break;
            }

            const ProcessValidationCommandMethod method = table[(size_t)type - 1];
            (this->*method)(begin, end);
        }
    }
}
