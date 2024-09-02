// © 2021 NVIDIA Corporation

#include "SharedMTL.h"

#include "CommandQueueMTL.h"

using namespace nri;

CommandQueueMTL::~CommandQueueMTL() {
    m_Handle = nil;
}

Result CommandQueueMTL::Create(CommandQueueType type, uint32_t familyIndex, id<MTLCommandQueue> handle) {
    m_Type = type;
    m_FamilyIndex = familyIndex;
    m_Handle = handle;
    return Result::SUCCESS;
}

inline void CommandQueueMTL::SetDebugName(const char* name) {
    [m_Handle setLabel:[NSString stringWithUTF8String:name]];
}

Result CommandQueueMTL::WaitForIdle() {

    id<MTLCommandBuffer> waitCmdBuf = [m_Handle commandBufferWithUnretainedReferences];

    [waitCmdBuf commit];
    [waitCmdBuf waitUntilCompleted];

    waitCmdBuf = nil;
}
