#pragma once

namespace nri {

struct DeviceMetal final : public DeviceBase {
    BufferMetal(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator);
    ~BufferMetal();

    Result FillFunctionTable(CoreInterface& table) const;
    Result FillFunctionTable(HelperInterface& table) const;
    Result FillFunctionTable(LowLatencyInterface& table) const;
    Result FillFunctionTable(MeshShaderInterface& table) const;
    Result FillFunctionTable(RayTracingInterface& table) const;
    Result FillFunctionTable(StreamerInterface& table) const;
    Result FillFunctionTable(SwapChainInterface& table) const;
    Result FillFunctionTable(ResourceAllocatorInterface& table) const;
    Result FillFunctionTable(WrapperD3D11Interface& table) const;
    Result FillFunctionTable(WrapperD3D12Interface& table) const;
    Result FillFunctionTable(WrapperVKInterface& table) const;

    //================================================================================================================
    // DeviceBase
    //================================================================================================================

    inline const DeviceDesc& GetDesc() const {
        return m_Desc;
    }

    Result Create(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationMTLDesc& deviceCreationVKDesc, bool isWrapper);
private:
    DeviceDesc m_Desc = {};
    id<MTLDevice>     m_Device;
    DeviceDesc m_Desc = {};
    bool m_OwnsNativeObjects = true;
};
}; // namespace nri
