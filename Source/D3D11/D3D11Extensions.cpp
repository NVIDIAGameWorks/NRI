// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"
#include "D3D11Extensions.h"

using namespace nri;

D3D11Extensions::~D3D11Extensions()
{
    if (m_IsNvAPIAvailable)
        NvAPI_Unload();

    if (m_AGSContext)
    {
        if (!m_IsImported)
            m_AGS.DeInit(m_AGSContext);
        m_AGSContext = nullptr;

        FreeLibrary(m_AGSLibrary);
        m_AGSLibrary = nullptr;
    }
}

void D3D11Extensions::InitializeNVExt(const nri::DeviceBase* deviceBase, bool isNVAPILoadedInApp, bool isImported)
{
    m_DeviceBase = deviceBase;
    m_IsImported = isImported;

    if (isImported && !isNVAPILoadedInApp)
        REPORT_WARNING(m_DeviceBase, "NVAPI is disabled, because it's not loaded on the application side");
    else
    {
        const NvAPI_Status status = NvAPI_Initialize();
        m_IsNvAPIAvailable = (status == NVAPI_OK);
        if (!m_IsNvAPIAvailable)
            REPORT_ERROR(m_DeviceBase, "Failed to initialize NVAPI: %d", (int32_t)status);
    }
}

void D3D11Extensions::InitializeAMDExt(const nri::DeviceBase* deviceBase, AGSContext* agsContext, bool isImported)
{
    m_DeviceBase = deviceBase;
    m_IsImported = isImported;

    if (isImported && !agsContext)
    {
        REPORT_WARNING(m_DeviceBase, "AMDAGS is disabled, because 'agsContext' is not provided");
        return;
    }

    m_AGSLibrary = LoadLibraryW(L"amd_ags_x64");
    if (!m_AGSLibrary)
    {
        REPORT_WARNING(m_DeviceBase, "AMDAGS is disabled, because 'amd_ags_x64' is not found");
        return;
    }

    m_AGS.Init = (PFN_agsInit)GetProcAddress(m_AGSLibrary, "agsInit");
    m_AGS.DeInit = (PFN_agsDeInit)GetProcAddress(m_AGSLibrary, "agsDeInit");
    m_AGS.CreateDevice = (PFN_agsDriverExtensionsDX11_CreateDevice)GetProcAddress(m_AGSLibrary, "agsDriverExtensionsDX11_CreateDevice");
    m_AGS.BeginUAVOverlap = (PFN_agsDriverExtensionsDX11_BeginUAVOverlap)GetProcAddress(m_AGSLibrary, "agsDriverExtensionsDX11_BeginUAVOverlap");
    m_AGS.EndUAVOverlap = (PFN_agsDriverExtensionsDX11_EndUAVOverlap)GetProcAddress(m_AGSLibrary, "agsDriverExtensionsDX11_EndUAVOverlap");
    m_AGS.MultiDrawIndexedInstancedIndirect = (PFN_agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect)GetProcAddress(m_AGSLibrary, "agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect");
    m_AGS.SetDepthBounds = (PFN_agsDriverExtensionsDX11_SetDepthBounds)GetProcAddress(m_AGSLibrary, "agsDriverExtensionsDX11_SetDepthBounds");
    m_AGS.MultiDrawInstancedIndirect = (PFN_agsDriverExtensionsDX11_MultiDrawInstancedIndirect)GetProcAddress(m_AGSLibrary, "agsDriverExtensionsDX11_MultiDrawInstancedIndirect");

    const void** functionArray = (const void**)&m_AGS;
    const size_t functionArraySize = sizeof(AGSFunctionTable) / sizeof(void*);
    size_t i = 0;
    for (; i < functionArraySize && functionArray[i] != nullptr; i++);

    if (i != functionArraySize)
    {
        REPORT_WARNING(m_DeviceBase, "AMDAGS is disabled, because not all functions are found in the DLL");
        FreeLibrary(m_AGSLibrary);
        m_AGSLibrary = nullptr;
        
        return;
    }

    if (!agsContext)
    {
        const AGSReturnCode result = m_AGS.Init(&agsContext, nullptr, nullptr);
        if (result != AGS_SUCCESS || !agsContext)
        {
            REPORT_ERROR(m_DeviceBase, "Failed to initialize AMDAGS: %d", (int32_t)result);
            FreeLibrary(m_AGSLibrary);
            m_AGSLibrary = nullptr;
        }
    }

    m_AGSContext = agsContext;
}

void D3D11Extensions::BeginUAVOverlap(ID3D11DeviceContext* deviceContext) const
{
    if (m_IsNvAPIAvailable)
    {
        const NvAPI_Status res = NvAPI_D3D11_BeginUAVOverlap(deviceContext);
        CHECK(res == NVAPI_OK, "NvAPI_D3D11_BeginUAVOverlap() - FAILED!");
    }
    else if (m_AGSContext)
    {
        const AGSReturnCode res = m_AGS.BeginUAVOverlap(m_AGSContext, deviceContext);
        CHECK(res == AGS_SUCCESS, "agsDriverExtensionsDX11_BeginUAVOverlap() - FAILED!");
    }
}

void D3D11Extensions::EndUAVOverlap(ID3D11DeviceContext* deviceContext) const
{
    if (m_IsNvAPIAvailable)
    {
        const NvAPI_Status status = NvAPI_D3D11_EndUAVOverlap(deviceContext);
        CHECK(status == NVAPI_OK, "NvAPI_D3D11_EndUAVOverlap() - FAILED!");
    }
    else if (m_AGSContext)
    {
        const AGSReturnCode res = m_AGS.EndUAVOverlap(m_AGSContext, deviceContext);
        CHECK(res == AGS_SUCCESS, "agsDriverExtensionsDX11_EndUAVOverlap() - FAILED!");
    }
}

void D3D11Extensions::WaitForDrain(ID3D11DeviceContext* deviceContext, uint32_t flags) const
{
    if (m_IsNvAPIAvailable)
    {
        const NvAPI_Status res = NvAPI_D3D11_BeginUAVOverlapEx(deviceContext, flags);
        CHECK(res == NVAPI_OK, "NvAPI_D3D11_BeginUAVOverlap() - FAILED!");
    }
    else if (m_AGSContext)
    {
        REPORT_WARNING(m_DeviceBase, "Verify that this code actually works on AMD!");

        const AGSReturnCode res1 = m_AGS.EndUAVOverlap(m_AGSContext, deviceContext);
        CHECK(res1 == AGS_SUCCESS, "agsDriverExtensionsDX11_EndUAVOverlap() - FAILED!");
        const AGSReturnCode res2 = m_AGS.BeginUAVOverlap(m_AGSContext, deviceContext);
        CHECK(res2 == AGS_SUCCESS, "agsDriverExtensionsDX11_BeginUAVOverlap() - FAILED!");
    }
}

void D3D11Extensions::SetDepthBounds(ID3D11DeviceContext* deviceContext, float minBound, float maxBound) const
{
    bool isEnabled = minBound != 0.0f || maxBound != 1.0f;

    if (m_IsNvAPIAvailable)
    {
        const NvAPI_Status status = NvAPI_D3D11_SetDepthBoundsTest(deviceContext, isEnabled, minBound, maxBound);
        CHECK(status == NVAPI_OK, "NvAPI_D3D11_SetDepthBoundsTest() - FAILED!");
    }
    else if (m_AGSContext)
    {
        const AGSReturnCode res = m_AGS.SetDepthBounds(m_AGSContext, deviceContext, isEnabled, minBound, maxBound);
        CHECK(res == AGS_SUCCESS, "agsDriverExtensionsDX11_SetDepthBounds() - FAILED!");
    }
}

void D3D11Extensions::MultiDrawIndirect(ID3D11DeviceContext* deviceContext, ID3D11Buffer* buffer, uint64_t offset, uint32_t drawNum, uint32_t stride) const
{
    if (m_IsNvAPIAvailable)
    {
        const NvAPI_Status status = NvAPI_D3D11_MultiDrawInstancedIndirect(deviceContext, drawNum, buffer, (uint32_t)offset, stride);
        CHECK(status == NVAPI_OK, "NvAPI_D3D11_MultiDrawInstancedIndirect() - FAILED!");
    }
    else if (m_AGSContext)
    {
        const AGSReturnCode res = m_AGS.MultiDrawInstancedIndirect(m_AGSContext, deviceContext, drawNum, buffer, (uint32_t)offset, stride);
        CHECK(res == AGS_SUCCESS, "agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect() - FAILED!");
    }
    else
    {
        for (uint32_t i = 0; i < drawNum; i++)
        {
            deviceContext->DrawInstancedIndirect(buffer, (uint32_t)offset);
            offset += stride;
        }
    }
}

void D3D11Extensions::MultiDrawIndexedIndirect(ID3D11DeviceContext* deviceContext, ID3D11Buffer* buffer, uint64_t offset, uint32_t drawNum, uint32_t stride) const
{
    if (m_IsNvAPIAvailable)
    {
        const NvAPI_Status status = NvAPI_D3D11_MultiDrawIndexedInstancedIndirect(deviceContext, drawNum, buffer, (uint32_t)offset, stride);
        CHECK(status == NVAPI_OK, "NvAPI_D3D11_MultiDrawInstancedIndirect() - FAILED!");
    }
    else if (m_AGSContext)
    {
        const AGSReturnCode res = m_AGS.MultiDrawIndexedInstancedIndirect(m_AGSContext, deviceContext, drawNum, buffer, (uint32_t)offset, stride);
        CHECK(res == AGS_SUCCESS, "agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect() - FAILED!");
    }
    else
    {
        for (uint32_t i = 0; i < drawNum; i++)
        {
            deviceContext->DrawIndexedInstancedIndirect(buffer, (uint32_t)offset);
            offset += stride;
        }
    }
}

void D3D11Extensions::CreateDeviceUsingAGS(IDXGIAdapter* adapter, const D3D_FEATURE_LEVEL* featureLevels, const size_t featureLevelNum, UINT flags, AGSDX11ReturnedParams& params)
{
    CHECK(m_AGSContext != nullptr, "Can't create a device using AGS: AGS is not available.");

    AGSDX11DeviceCreationParams deviceCreationParams = {};
    deviceCreationParams.pAdapter = adapter;
    deviceCreationParams.DriverType = D3D_DRIVER_TYPE_UNKNOWN;
    deviceCreationParams.Flags = flags;
    deviceCreationParams.pFeatureLevels = featureLevels;
    deviceCreationParams.FeatureLevels = (uint32_t)featureLevelNum;
    deviceCreationParams.SDKVersion = D3D11_SDK_VERSION;

    AGSDX11ExtensionParams extensionsParams = {};
    extensionsParams.uavSlot = 63; // TODO: move to a header, share with NVAPI

    AGSReturnCode result = m_AGS.CreateDevice(m_AGSContext, &deviceCreationParams, &extensionsParams, &params);

    if (flags != 0 && result != AGS_SUCCESS)
    {
        deviceCreationParams.Flags = 0;
        result = m_AGS.CreateDevice(m_AGSContext, &deviceCreationParams, &extensionsParams, &params);
    }

    RETURN_ON_FAILURE(m_DeviceBase, result == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_CreateDevice() failed: %d", (int32_t)result);
}