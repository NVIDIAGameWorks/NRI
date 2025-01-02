#include "SharedMTL.h"

#include "CommandAllocatorMTL.h"
#include "CommandBufferMTL.h"
#include "CommandQueueMTL.h"

using namespace nri;
    
CommandAllocatorMTL::~CommandAllocatorMTL() {

}

Result CommandAllocatorMTL::Create(const CommandQueue& commandQueue) {
    const CommandQueueMTL& commandQueueImpl = (CommandQueueMTL&)commandQueue;
    m_CommandQueue = &commandQueueImpl;
    return Result::SUCCESS;
}


void CommandAllocatorMTL::SetDebugName(const char* name) {
    
}

void CommandAllocatorMTL::Reset() {
    
}
//================================================================================================================
// NRI
//================================================================================================================

Result CommandAllocatorMTL::CreateCommandBuffer(CommandBuffer*& commandBuffer) {

    CommandBufferMTL* commandBufferImpl = Allocate<CommandBufferMTL>(m_Device.GetStdAllocator(), m_Device);
    commandBufferImpl->Create(m_CommandQueue);
    commandBuffer = (CommandBuffer*)commandBufferImpl;
    return Result::SUCCESS;
}

#include "CommandAllocatorMTL.hpp"
