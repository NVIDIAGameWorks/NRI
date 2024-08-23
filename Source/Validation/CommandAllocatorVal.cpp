// © 2021 NVIDIA Corporation

#include "SharedExternal.h"
#include "SharedVal.h"

#include "CommandAllocatorVal.h"
#include "CommandBufferVal.h"

using namespace nri;

void CommandAllocatorVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetCommandAllocatorDebugName(*GetImpl(), name);
}

Result CommandAllocatorVal::CreateCommandBuffer(CommandBuffer*& commandBuffer) {
    CommandBuffer* commandBufferImpl;
    const Result result = GetCoreInterface().CreateCommandBuffer(*GetImpl(), commandBufferImpl);

    if (result == Result::SUCCESS)
        commandBuffer = (CommandBuffer*)Allocate<CommandBufferVal>(m_Device.GetStdAllocator(), m_Device, commandBufferImpl, false);

    return result;
}

void CommandAllocatorVal::Reset() {
    GetCoreInterface().ResetCommandAllocator(*GetImpl());
}

#include "CommandAllocatorVal.hpp"
