#pragma once

#include "amdags/ags_lib/inc/amd_ags.h"
#include "nvapi/nvapi.h"

typedef AGSReturnCode (*PFN_agsInit)(AGSContext** agsContext, const AGSConfiguration* config, AGSGPUInfo* gpuInfo);
typedef AGSReturnCode (*PFN_agsDeInit)(AGSContext* agsContext);
typedef AGSReturnCode (*PFN_agsDriverExtensionsDX11_CreateDevice)(AGSContext* agsContext, const AGSDX11DeviceCreationParams* creationParams, const AGSDX11ExtensionParams* extensionParams, AGSDX11ReturnedParams* returnedParams);
typedef AGSReturnCode (*PFN_agsDriverExtensionsDX11_BeginUAVOverlap)(AGSContext* agsContext, ID3D11DeviceContext* deviceContext);
typedef AGSReturnCode (*PFN_agsDriverExtensionsDX11_EndUAVOverlap)(AGSContext* agsContext, ID3D11DeviceContext* deviceContext);
typedef AGSReturnCode (*PFN_agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect)(AGSContext* agsContext, ID3D11DeviceContext* deviceContext, unsigned int drawCount, ID3D11Buffer* pBufferForArgs, unsigned int alignedByteOffsetForArgs, unsigned int byteStrideForArgs);
typedef AGSReturnCode (*PFN_agsDriverExtensionsDX11_SetDepthBounds)(AGSContext* agsContext, ID3D11DeviceContext* deviceContext, bool enabled, float minDepth, float maxDepth);
typedef AGSReturnCode (*PFN_agsDriverExtensionsDX11_MultiDrawInstancedIndirect)(AGSContext* agsContext, ID3D11DeviceContext* deviceContext, unsigned int drawCount, ID3D11Buffer* pBufferForArgs, unsigned int alignedByteOffsetForArgs, unsigned int byteStrideForArgs);

namespace nri
{

struct AGSFunctionTable
{
    PFN_agsInit Init;
    PFN_agsDeInit DeInit;
    PFN_agsDriverExtensionsDX11_CreateDevice CreateDevice;
    PFN_agsDriverExtensionsDX11_BeginUAVOverlap BeginUAVOverlap;
    PFN_agsDriverExtensionsDX11_EndUAVOverlap EndUAVOverlap;
    PFN_agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect MultiDrawIndexedInstancedIndirect;
    PFN_agsDriverExtensionsDX11_SetDepthBounds SetDepthBounds;
    PFN_agsDriverExtensionsDX11_MultiDrawInstancedIndirect MultiDrawInstancedIndirect;
};

struct D3D11Extensions
{
    ~D3D11Extensions();

    inline bool IsNvAPIAvailable() const
    { return m_IsNvAPIAvailable; }

    inline bool IsAGSAvailable() const
    { return m_AGSContext != nullptr; }

    void InitializeNVExt(const nri::DeviceBase* deviceBase, bool isNVAPILoadedInApp, bool isImported);
    void InitializeAMDExt(const nri::DeviceBase* deviceBase, AGSContext* agsContext, bool isImported);

    void BeginUAVOverlap(ID3D11DeviceContext* deviceContext) const;
    void EndUAVOverlap(ID3D11DeviceContext* deviceContext) const;
    void WaitForDrain(ID3D11DeviceContext* deviceContext, uint32_t flags) const;
    void SetDepthBounds(ID3D11DeviceContext* deviceContext, float minBound, float maxBound) const;
    void MultiDrawIndirect(ID3D11DeviceContext* deviceContext, ID3D11Buffer* buffer, uint64_t offset, uint32_t drawNum, uint32_t stride) const;
    void MultiDrawIndexedIndirect(ID3D11DeviceContext* deviceContext, ID3D11Buffer* buffer, uint64_t offset, uint32_t drawNum, uint32_t stride) const;

    ID3D11Device* CreateDeviceUsingAGS(IDXGIAdapter* adapter, const D3D_FEATURE_LEVEL* featureLevels, const size_t featureLevelNum, UINT flags);

    const nri::DeviceBase* m_DeviceBase = nullptr;
    AGSContext* m_AGSContext = nullptr;
    AGSFunctionTable m_AGS = {};
    HMODULE m_AGSLibrary = nullptr;
    bool m_IsNvAPIAvailable = false;
    bool m_IsImported = false;
};

}
