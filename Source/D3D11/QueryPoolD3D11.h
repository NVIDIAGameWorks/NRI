// © 2021 NVIDIA Corporation

#pragma once

namespace nri
{

struct DeviceD3D11;

struct QueryPoolD3D11
{
    inline QueryPoolD3D11(DeviceD3D11& device) :
        m_Device(device)
        , m_QueryPool(device.GetStdAllocator())
    {}

    inline ~QueryPoolD3D11()
    {}

    inline DeviceD3D11& GetDevice() const
    { return m_Device; }

    Result Create(const QueryPoolDesc& queryPoolDesc);
    void BeginQuery(ID3D11DeviceContextBest* deferredContext, uint32_t offset);
    void EndQuery(ID3D11DeviceContextBest* deferredContext, uint32_t offset);
    void GetData(uint8_t* dstMemory, uint32_t offset, uint32_t num) const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    uint32_t GetQuerySize() const;

private:
    DeviceD3D11& m_Device;
    Vector<ComPtr<ID3D11Query>> m_QueryPool;
    QueryType m_Type = QueryType::MAX_NUM;
};

}
