// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"
#include "FenceD3D11.h"

using namespace nri;

Result FenceD3D11::Create(uint64_t initialValue)
{
    if (m_Device.GetVersion() >= 5)
    {
        HRESULT hr = m_Device->CreateFence(initialValue, D3D11_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device5::CreateFence()");
    }
    else
    {
        D3D11_QUERY_DESC queryDesc = {};
        queryDesc.Query = D3D11_QUERY_EVENT;

        HRESULT hr = m_Device->CreateQuery(&queryDesc, &m_Query);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateQuery()");
    }

    m_Event = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    m_Value = initialValue;

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline uint64_t FenceD3D11::GetFenceValue() const
{
    if (m_Fence)
        return m_Fence->GetCompletedValue();

    return m_Value;
}

inline void FenceD3D11::QueueSignal(CommandQueueD3D11& commandQueue, uint64_t value)
{
    MaybeUnused(commandQueue);

    if (m_Fence)
    {
        HRESULT hr = m_Device.GetImmediateContext()->Signal(m_Fence, value);
        CHECK(&m_Device, hr == S_OK, "D3D11DeviceContext4::Signal() - FAILED!");
    }
    else
    {
        m_Device.GetImmediateContext()->End(m_Query);
        m_Value = value;
    }
}

inline void FenceD3D11::QueueWait(CommandQueueD3D11& commandQueue, uint64_t value)
{
    MaybeUnused(commandQueue);

    if (m_Fence)
    {
        HRESULT hr = m_Device.GetImmediateContext()->Wait(m_Fence, value);
        CHECK(&m_Device, hr == S_OK, "D3D11DeviceContext4::Wait() - FAILED!");
    }
}

inline void FenceD3D11::Wait(uint64_t value)
{
    if (m_Fence)
    {
        if (m_Event == 0 || m_Event == INVALID_HANDLE_VALUE)
        {
            while (m_Fence->GetCompletedValue() < value)
                ;
        }
        else if (m_Fence->GetCompletedValue() < value)
        {
            HRESULT hr = m_Fence->SetEventOnCompletion(value, m_Event);
            CHECK(&m_Device, hr == S_OK, "ID3D12Fence::SetEventOnCompletion() - FAILED!");

            uint32_t result = WaitForSingleObjectEx(m_Event, DEFAULT_TIMEOUT, TRUE);
            CHECK(&m_Device, result == WAIT_OBJECT_0, "WaitForSingleObjectEx(): failed, result = 0x%08X!", result);
        }
    }
    else
    {
        HRESULT hr = S_FALSE;
        while (hr == S_FALSE)
            hr = m_Device.GetImmediateContext()->GetData(m_Query, nullptr, 0, 0);

        CHECK(&m_Device, hr == S_OK, "D3D11DeviceContext::GetData() - FAILED!");
    }
}

#include "FenceD3D11.hpp"
