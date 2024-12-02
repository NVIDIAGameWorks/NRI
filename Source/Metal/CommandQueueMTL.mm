// © 2021 NVIDIA Corporation

#include "SharedMTL.h"

#include "CommandQueueMTL.h"
#include "CommandBufferMTL.h"

using namespace nri;

CommandQueueMTL::~CommandQueueMTL() {
    m_Handle = nil;
}

Result CommandQueueMTL::Create(CommandQueueType type) {
    m_Type = type;
    const char* queueNames[] = {
        "GRAPHICS_QUEUE", // GRAPHICS
        "TRANSFER_QUEUE", // TRANSFER
        "COMPUTE_QUEUE" // COMPUTE
    };

    m_Handle = [m_Device newCommandQueueWithMaxCommandBufferCount:512];
    SetDebugName(queueNames[(uint32_t)type]);
    
    return Result::SUCCESS;
}


void CommandQueueMTL::Submit(const QueueSubmitDesc& queueSubmitDesc, const SwapChain* swapChain) {
    
    for(uint32_t i = 0; i < queueSubmitDesc.commandBufferNum; i++) {
        id<MTLCommandBuffer> cmd = *(struct CommandBufferMTL*)queueSubmitDesc.commandBuffers[i];
        [cmd commit];
        
    }
    
    
}


void CommandQueueMTL::SetDebugName(const char* name) {
    [m_Handle setLabel:[NSString stringWithUTF8String:name]];
}

Result CommandQueueMTL::WaitForIdle() {

    id<MTLCommandBuffer> waitCmdBuf = [m_Handle commandBufferWithUnretainedReferences];

    [waitCmdBuf commit];
    [waitCmdBuf waitUntilCompleted];

    waitCmdBuf = nil;
}
