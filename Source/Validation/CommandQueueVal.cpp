// © 2021 NVIDIA Corporation

#include "SharedExternal.h"
#include "SharedVal.h"
#include "CommandQueueVal.h"

#include "CommandBufferVal.h"
#include "TextureVal.h"
#include "BufferVal.h"
#include "QueryPoolVal.h"

using namespace nri;

static bool ValidateTransitionBarrierDesc(DeviceVal& device, uint32_t i, const BufferTransitionBarrierDesc& bufferTransitionBarrierDesc) {
    const BufferVal& bufferVal = *(BufferVal*)bufferTransitionBarrierDesc.buffer;
    const BufferUsageBits usageMask = bufferVal.GetDesc().usageMask;

    RETURN_ON_FAILURE(&device, bufferTransitionBarrierDesc.buffer != nullptr, false, "ChangeResourceStates: 'transitionBarriers.buffers[%u].buffer' is NULL", i);
    RETURN_ON_FAILURE(&device, bufferVal.IsBoundToMemory(), false, "ChangeResourceStates: 'transitionBarriers.buffers[%u].buffer' is not bound to memory", i);

    RETURN_ON_FAILURE(
        &device, IsAccessMaskSupported(usageMask, bufferTransitionBarrierDesc.prevAccess), false,
        "ChangeResourceStates: 'transitionBarriers.buffers[%u].prevAccess' is not supported by usageMask of the buffer", i
    );

    RETURN_ON_FAILURE(
        &device, IsAccessMaskSupported(usageMask, bufferTransitionBarrierDesc.nextAccess), false,
        "ChangeResourceStates: 'transitionBarriers.buffers[%u].nextAccess' is not supported by usageMask of the buffer", i
    );

    return true;
}

static bool ValidateTransitionBarrierDesc(DeviceVal& device, uint32_t i, const TextureTransitionBarrierDesc& textureTransitionBarrierDesc) {
    const TextureVal& textureVal = *(TextureVal*)textureTransitionBarrierDesc.texture;
    const TextureUsageBits usageMask = textureVal.GetDesc().usageMask;

    RETURN_ON_FAILURE(&device, textureTransitionBarrierDesc.texture != nullptr, false, "ChangeResourceStates: 'transitionBarriers.textures[%u].texture' is NULL", i);
    RETURN_ON_FAILURE(&device, textureVal.IsBoundToMemory(), false, "ChangeResourceStates: 'transitionBarriers.textures[%u].texture' is not bound to memory", i);

    RETURN_ON_FAILURE(
        &device, IsAccessMaskSupported(usageMask, textureTransitionBarrierDesc.prevState.acessBits), false,
        "ChangeResourceStates: 'transitionBarriers.textures[%u].prevAccess' is not supported by usageMask of the texture", i
    );

    RETURN_ON_FAILURE(
        &device, IsAccessMaskSupported(usageMask, textureTransitionBarrierDesc.nextState.acessBits), false,
        "ChangeResourceStates: 'transitionBarriers.textures[%u].nextAccess' is not supported by usageMask of the texture", i
    );

    RETURN_ON_FAILURE(
        &device, IsTextureLayoutSupported(usageMask, textureTransitionBarrierDesc.prevState.layout), false,
        "ChangeResourceStates: 'transitionBarriers.textures[%u].prevLayout' is not supported by usageMask of the texture", i
    );

    RETURN_ON_FAILURE(
        &device, IsTextureLayoutSupported(usageMask, textureTransitionBarrierDesc.nextState.layout), false,
        "ChangeResourceStates: 'transitionBarriers.textures[%u].nextLayout' is not supported by usageMask of the texture", i
    );

    return true;
}

static bool ValidateTextureUploadDesc(DeviceVal& device, uint32_t i, const TextureUploadDesc& textureUploadDesc) {
    const uint32_t subresourceNum = textureUploadDesc.arraySize * textureUploadDesc.mipNum;
    if (subresourceNum == 0 && textureUploadDesc.subresources != nullptr) {
        REPORT_WARNING(&device, "UploadData: the number of subresources in 'textureUploadDescs[%u]' is 0 (nothing to upload)", i);
        return true;
    }

    if (textureUploadDesc.subresources == nullptr)
        return true;

    const TextureVal& textureVal = *(TextureVal*)textureUploadDesc.texture;

    RETURN_ON_FAILURE(&device, textureUploadDesc.texture != nullptr, false, "UploadData: 'textureUploadDescs[%u].texture' is NULL", i);
    RETURN_ON_FAILURE(&device, textureUploadDesc.mipNum <= textureVal.GetDesc().mipNum, false, "UploadData: 'textureUploadDescs[%u].mipNum' is invalid", i);
    RETURN_ON_FAILURE(&device, textureUploadDesc.arraySize <= textureVal.GetDesc().arraySize, false, "UploadData: 'textureUploadDescs[%u].arraySize' is invalid", i);
    RETURN_ON_FAILURE(&device, textureUploadDesc.nextState.layout < TextureLayout::MAX_NUM, false, "UploadData: 'textureUploadDescs[%u].nextLayout' is invalid", i);
    RETURN_ON_FAILURE(&device, textureVal.IsBoundToMemory(), false, "UploadData: 'textureUploadDescs[%u].texture' is not bound to memory", i);

    for (uint32_t j = 0; j < subresourceNum; j++) {
        const TextureSubresourceUploadDesc& subresource = textureUploadDesc.subresources[j];

        if (subresource.sliceNum == 0) {
            REPORT_WARNING(
                &device,
                "No data to upload: the number of subresources in "
                "'textureUploadDescs[%u].subresources[%u].sliceNum' is 0",
                i, j
            );
            continue;
        }

        RETURN_ON_FAILURE(&device, subresource.slices != nullptr, false, "UploadData: 'textureUploadDescs[%u].subresources[%u].slices' is invalid", i, j);
        RETURN_ON_FAILURE(&device, subresource.rowPitch != 0, false, "UploadData: 'textureUploadDescs[%u].subresources[%u].rowPitch' is 0", i, j);
        RETURN_ON_FAILURE(&device, subresource.slicePitch != 0, false, "UploadData: 'textureUploadDescs[%u].subresources[%u].slicePitch' is 0", i, j);
    }

    return true;
}

static bool ValidateBufferUploadDesc(DeviceVal& device, uint32_t i, const BufferUploadDesc& bufferUploadDesc) {
    if (bufferUploadDesc.dataSize == 0) {
        REPORT_WARNING(&device, "UploadData: 'bufferUploadDescs[%u].dataSize' is 0 (nothing to upload)", i);
        return true;
    }

    const BufferVal& bufferVal = *(BufferVal*)bufferUploadDesc.buffer;
    const uint64_t rangeEnd = bufferUploadDesc.bufferOffset + bufferUploadDesc.dataSize;

    RETURN_ON_FAILURE(&device, bufferUploadDesc.buffer != nullptr, false, "UploadData: 'bufferUploadDescs[%u].buffer' is invalid", i);
    RETURN_ON_FAILURE(&device, bufferUploadDesc.data != nullptr, false, "UploadData: 'bufferUploadDescs[%u].data' is invalid", i);
    RETURN_ON_FAILURE(&device, bufferVal.IsBoundToMemory(), false, "UploadData: 'bufferUploadDescs[%u].buffer' is not bound to memory", i);

    RETURN_ON_FAILURE(
        &device, rangeEnd <= bufferVal.GetDesc().size, false, "UploadData: 'bufferUploadDescs[%u].bufferOffset + bufferUploadDescs[%u].dataSize' is out of bounds", i, i
    );

    return true;
}

void CommandQueueVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetCommandQueueDebugName(*GetImpl(), name);
}

void CommandQueueVal::Submit(const QueueSubmitDesc& queueSubmitDesc) {
    ProcessValidationCommands((const CommandBufferVal* const*)queueSubmitDesc.commandBuffers, queueSubmitDesc.commandBufferNum);

    auto queueSubmitDescImpl = queueSubmitDesc;
    queueSubmitDescImpl.commandBuffers = STACK_ALLOC(CommandBuffer*, queueSubmitDesc.commandBufferNum);
    for (uint32_t i = 0; i < queueSubmitDesc.commandBufferNum; i++)
        ((CommandBuffer**)queueSubmitDescImpl.commandBuffers)[i] = NRI_GET_IMPL(CommandBuffer, queueSubmitDesc.commandBuffers[i]);

    GetCoreInterface().QueueSubmit(*GetImpl(), queueSubmitDescImpl);
}

Result CommandQueueVal::ChangeResourceStates(const TransitionBarrierDesc& transitionBarriers) {
    auto* bufferTransitionBarriers = STACK_ALLOC(BufferTransitionBarrierDesc, transitionBarriers.bufferNum);
    auto* textureTransitionBarriers = STACK_ALLOC(TextureTransitionBarrierDesc, transitionBarriers.textureNum);

    for (uint32_t i = 0; i < transitionBarriers.bufferNum; i++) {
        if (!ValidateTransitionBarrierDesc(m_Device, i, transitionBarriers.buffers[i]))
            return Result::INVALID_ARGUMENT;

        const BufferVal& bufferVal = *(BufferVal*)transitionBarriers.buffers[i].buffer;

        bufferTransitionBarriers[i] = transitionBarriers.buffers[i];
        bufferTransitionBarriers[i].buffer = bufferVal.GetImpl();
    }

    for (uint32_t i = 0; i < transitionBarriers.textureNum; i++) {
        if (!ValidateTransitionBarrierDesc(m_Device, i, transitionBarriers.textures[i]))
            return Result::INVALID_ARGUMENT;

        const TextureVal& textureVal = *(TextureVal*)transitionBarriers.textures[i].texture;

        textureTransitionBarriers[i] = transitionBarriers.textures[i];
        textureTransitionBarriers[i].texture = textureVal.GetImpl();
    }

    TransitionBarrierDesc transitionBarriersImpl = transitionBarriers;
    transitionBarriersImpl.buffers = bufferTransitionBarriers;
    transitionBarriersImpl.textures = textureTransitionBarriers;

    return m_HelperAPI.ChangeResourceStates(*GetImpl(), transitionBarriersImpl);
}

Result CommandQueueVal::UploadData(
    const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum
) {
    RETURN_ON_FAILURE(&m_Device, textureUploadDescNum == 0 || textureUploadDescs != nullptr, Result::INVALID_ARGUMENT, "UploadData: 'textureUploadDescs' is NULL");
    RETURN_ON_FAILURE(&m_Device, bufferUploadDescNum == 0 || bufferUploadDescs != nullptr, Result::INVALID_ARGUMENT, "UploadData: 'bufferUploadDescs' is NULL");

    TextureUploadDesc* textureUploadDescsImpl = STACK_ALLOC(TextureUploadDesc, textureUploadDescNum);

    for (uint32_t i = 0; i < textureUploadDescNum; i++) {
        if (!ValidateTextureUploadDesc(m_Device, i, textureUploadDescs[i]))
            return Result::INVALID_ARGUMENT;

        const TextureVal* textureVal = (TextureVal*)textureUploadDescs[i].texture;

        textureUploadDescsImpl[i] = textureUploadDescs[i];
        textureUploadDescsImpl[i].texture = textureVal->GetImpl();
    }

    BufferUploadDesc* bufferUploadDescsImpl = STACK_ALLOC(BufferUploadDesc, bufferUploadDescNum);

    for (uint32_t i = 0; i < bufferUploadDescNum; i++) {
        if (!ValidateBufferUploadDesc(m_Device, i, bufferUploadDescs[i]))
            return Result::INVALID_ARGUMENT;

        const BufferVal* bufferVal = (BufferVal*)bufferUploadDescs[i].buffer;

        bufferUploadDescsImpl[i] = bufferUploadDescs[i];
        bufferUploadDescsImpl[i].buffer = bufferVal->GetImpl();
    }

    return m_HelperAPI.UploadData(*GetImpl(), textureUploadDescsImpl, textureUploadDescNum, bufferUploadDescsImpl, bufferUploadDescNum);
}

Result CommandQueueVal::WaitForIdle() {
    return m_HelperAPI.WaitForIdle(*GetImpl());
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
    CHECK(&m_Device, command != nullptr, "ProcessValidationCommandBeginQuery() failed: can't parse command");
    CHECK(&m_Device, command->queryPool != nullptr, "ProcessValidationCommandBeginQuery() failed: query pool is invalid");

    QueryPoolVal& queryPool = *(QueryPoolVal*)command->queryPool;
    const bool used = queryPool.SetQueryState(command->queryPoolOffset, true);

    if (used)
        REPORT_ERROR(&m_Device, "CmdBeginQuery: it must be reset before use. (QueryPool='%s', offset=%u)", queryPool.GetDebugName(), command->queryPoolOffset);
}

void CommandQueueVal::ProcessValidationCommandEndQuery(const uint8_t*& begin, const uint8_t* end) {
    const ValidationCommandUseQuery* command = ReadCommand<ValidationCommandUseQuery>(begin, end);
    CHECK(&m_Device, command != nullptr, "ProcessValidationCommandEndQuery() failed: can't parse command");
    CHECK(&m_Device, command->queryPool != nullptr, "ProcessValidationCommandEndQuery() failed: query pool is invalid");

    QueryPoolVal& queryPool = *(QueryPoolVal*)command->queryPool;
    const bool used = queryPool.SetQueryState(command->queryPoolOffset, true);

    if (queryPool.GetQueryType() == QueryType::TIMESTAMP) {
        if (used)
            REPORT_ERROR(&m_Device, "CmdEndQuery: it must be reset before use. (QueryPool='%s', offset=%u)", queryPool.GetDebugName(), command->queryPoolOffset);
    } else {
        if (!used)
            REPORT_ERROR(&m_Device, "CmdEndQuery: it's not in active state. (QueryPool='%s', offset=%u)", queryPool.GetDebugName(), command->queryPoolOffset);
    }
}

void CommandQueueVal::ProcessValidationCommandResetQuery(const uint8_t*& begin, const uint8_t* end) {
    const ValidationCommandResetQuery* command = ReadCommand<ValidationCommandResetQuery>(begin, end);
    CHECK(&m_Device, command != nullptr, "ProcessValidationCommandResetQuery() failed: can't parse command");
    CHECK(&m_Device, command->queryPool != nullptr, "ProcessValidationCommandResetQuery() failed: query pool is invalid");

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

#include "CommandQueueVal.hpp"
