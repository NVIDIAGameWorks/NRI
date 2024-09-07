#include "SharedMTL.h"

#include "CommandAllocatorMTL.h"

#include "CommandQueueMTL.h"

using namespace nri;
    
CommandAllocatorMTL::~CommandAllocatorMTL() {

}

Result CommandAllocatorMTL::Create(const CommandQueue& commandQueue) {
    m_CommandQueue = &(CommandQueueMTL&)commandQueue;
    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

Result CommandAllocatorMTL::CreateCommandBuffer(CommandBuffer*& commandBuffer) {
    

    return Result::SUCCESS;
}

#include "CommandAllocatorMTL.hpp"
