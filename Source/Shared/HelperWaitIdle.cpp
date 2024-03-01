#include "SharedExternal.h"

using namespace nri;

HelperWaitIdle::HelperWaitIdle(const CoreInterface& NRI, Device& device, CommandQueue& commandQueue) : NRI(NRI), m_Device(device), m_CommandQueue(commandQueue) {
    NRI.CreateFence(device, 0, m_Fence);
}

HelperWaitIdle::~HelperWaitIdle() {
    if (m_Fence)
        NRI.DestroyFence(*m_Fence);
}

Result HelperWaitIdle::WaitIdle() {
    if (!m_Fence)
        return Result::FAILURE;

    NRI.QueueSignal(m_CommandQueue, *m_Fence, 1);
    NRI.Wait(*m_Fence, 1);

    return Result::SUCCESS;
}
