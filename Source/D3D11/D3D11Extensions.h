#pragma once

#include "amdags/ags_lib/inc/amd_ags.h"
#include "nvapi/nvapi.h"

namespace nri {

struct AGSFunctionTable {
    AGS_INITIALIZE Initialize;
    AGS_DEINITIALIZE Deinitialize;
    AGS_DRIVEREXTENSIONSDX11_CREATEDEVICE CreateDevice;
    AGS_DRIVEREXTENSIONSDX11_BEGINUAVOVERLAP BeginUAVOverlap;
    AGS_DRIVEREXTENSIONSDX11_ENDUAVOVERLAP EndUAVOverlap;
    AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINDEXEDINSTANCEDINDIRECT MultiDrawIndexedInstancedIndirect;
    AGS_DRIVEREXTENSIONSDX11_SETDEPTHBOUNDS SetDepthBounds;
    AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINSTANCEDINDIRECT MultiDrawInstancedIndirect;
};

struct D3D11Extensions {
    ~D3D11Extensions();

    inline bool HasNVAPI() const {
        return m_IsNvAPIAvailable;
    }

    inline bool HasAGS() const {
        return m_AGSContext != nullptr;
    }

    void InitializeNVExt(const nri::DeviceBase* deviceBase, bool isNVAPILoadedInApp, bool isImported);
    void InitializeAMDExt(const nri::DeviceBase* deviceBase, AGSContext* agsContext, bool isImported);

    void BeginUAVOverlap(ID3D11DeviceContext* deviceContext) const;
    void EndUAVOverlap(ID3D11DeviceContext* deviceContext) const;
    void WaitForDrain(ID3D11DeviceContext* deviceContext, uint32_t flags) const;
    void SetDepthBounds(ID3D11DeviceContext* deviceContext, float minBound, float maxBound) const;
    void MultiDrawIndirect(ID3D11DeviceContext* deviceContext, ID3D11Buffer* buffer, uint64_t offset, uint32_t drawNum, uint32_t stride) const;
    void MultiDrawIndexedIndirect(ID3D11DeviceContext* deviceContext, ID3D11Buffer* buffer, uint64_t offset, uint32_t drawNum, uint32_t stride) const;

    void CreateDeviceUsingAGS(IDXGIAdapter* adapter, const D3D_FEATURE_LEVEL* featureLevels, size_t featureLevelNum, UINT flags, AGSDX11ReturnedParams& params);

    const nri::DeviceBase* m_DeviceBase = nullptr;
    AGSContext* m_AGSContext = nullptr;
    AGSFunctionTable m_AGS = {};
    HMODULE m_AGSLibrary = nullptr;
    bool m_IsNvAPIAvailable = false;
    bool m_IsImported = false;
};

} // namespace nri
