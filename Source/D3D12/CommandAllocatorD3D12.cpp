// © 2021 NVIDIA Corporation

#include "SharedD3D12.h"
#include "CommandAllocatorD3D12.h"
#include "CommandQueueD3D12.h"
#include "CommandBufferD3D12.h"

using namespace nri;

Result CommandAllocatorD3D12::Create(const CommandQueue& commandQueue)
{
    const CommandQueueD3D12& commandQueueD3D12 = (CommandQueueD3D12&)commandQueue;
    m_CommandListType = commandQueueD3D12.GetType();
    HRESULT hr = m_Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&m_CommandAllocator));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommandAllocator()");

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline Result CommandAllocatorD3D12::CreateCommandBuffer(CommandBuffer*& commandBuffer)
{
    CommandBufferD3D12* commandBufferD3D12 = Allocate<CommandBufferD3D12>(m_Device.GetStdAllocator(), m_Device);
    const Result result = commandBufferD3D12->Create(m_CommandListType, m_CommandAllocator);

    if (result == Result::SUCCESS)
    {
        commandBuffer = (CommandBuffer*)commandBufferD3D12;
        return Result::SUCCESS;
    }

    Deallocate(m_Device.GetStdAllocator(), commandBufferD3D12);

    return result;
}

inline void CommandAllocatorD3D12::Reset()
{
    m_CommandAllocator->Reset();
}

#include "CommandAllocatorD3D12.hpp"
