// © 2021 NVIDIA Corporation

Ext::~Ext() {
    if (m_IsNvAPIAvailable)
        NvAPI_Unload();

    if (m_AGSContext) {
        if (!m_IsImported)
            m_AGS.Deinitialize(m_AGSContext);
        m_AGSContext = nullptr;

        UnloadSharedLibrary(*m_AGSLibrary);
        m_AGSLibrary = nullptr;
    }
}

void Ext::InitializeNVExt(const nri::DeviceBase* deviceBase, bool isNVAPILoadedInApp, bool isImported) {
    m_DeviceBase = deviceBase;
    m_IsImported = isImported;

    if (isImported && !isNVAPILoadedInApp)
        REPORT_WARNING(m_DeviceBase, "NVAPI is disabled, because it's not loaded on the application side");
    else {
        const NvAPI_Status status = NvAPI_Initialize();
        m_IsNvAPIAvailable = (status == NVAPI_OK);
        if (!m_IsNvAPIAvailable)
            REPORT_ERROR(m_DeviceBase, "Failed to initialize NVAPI: %d", (int32_t)status);
    }
}

void Ext::InitializeAMDExt(const nri::DeviceBase* deviceBase, AGSContext* agsContext, bool isImported) {
    m_DeviceBase = deviceBase;
    m_IsImported = isImported;

    if (isImported && !agsContext) {
        REPORT_WARNING(m_DeviceBase, "AMDAGS is disabled, because 'agsContext' is not provided");
        return;
    }

    m_AGSLibrary = LoadSharedLibrary("amd_ags_x64.dll");
    if (!m_AGSLibrary) {
        REPORT_WARNING(m_DeviceBase, "AMDAGS is disabled, because 'amd_ags_x64' is not found");
        return;
    }

    m_AGS.Initialize = (AGS_INITIALIZE)GetSharedLibraryFunction(*m_AGSLibrary, "agsInitialize");
    m_AGS.Deinitialize = (AGS_DEINITIALIZE)GetSharedLibraryFunction(*m_AGSLibrary, "agsDeInitialize");

    m_AGS.CreateDeviceD3D11 = (AGS_DRIVEREXTENSIONSDX11_CREATEDEVICE)GetSharedLibraryFunction(*m_AGSLibrary, "agsDriverExtensionsDX11_CreateDevice");
    m_AGS.DestroyDeviceD3D11 = (AGS_DRIVEREXTENSIONSDX11_DESTROYDEVICE)GetSharedLibraryFunction(*m_AGSLibrary, "agsDriverExtensionsDX11_DestroyDevice");
    m_AGS.BeginUAVOverlap = (AGS_DRIVEREXTENSIONSDX11_BEGINUAVOVERLAP)GetSharedLibraryFunction(*m_AGSLibrary, "agsDriverExtensionsDX11_BeginUAVOverlap");
    m_AGS.EndUAVOverlap = (AGS_DRIVEREXTENSIONSDX11_ENDUAVOVERLAP)GetSharedLibraryFunction(*m_AGSLibrary, "agsDriverExtensionsDX11_EndUAVOverlap");
    m_AGS.SetDepthBounds = (AGS_DRIVEREXTENSIONSDX11_SETDEPTHBOUNDS)GetSharedLibraryFunction(*m_AGSLibrary, "agsDriverExtensionsDX11_SetDepthBounds");
    m_AGS.DrawIndirect = (AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINSTANCEDINDIRECT)GetSharedLibraryFunction(*m_AGSLibrary, "agsDriverExtensionsDX11_MultiDrawInstancedIndirect");
    m_AGS.DrawIndexedIndirect =
        (AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINDEXEDINSTANCEDINDIRECT)GetSharedLibraryFunction(*m_AGSLibrary, "agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect");
    m_AGS.DrawIndirectCount = (AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINSTANCEDINDIRECTCOUNTINDIRECT)GetSharedLibraryFunction(
        *m_AGSLibrary, "agsDriverExtensionsDX11_MultiDrawInstancedIndirectCountIndirect");
    m_AGS.DrawIndexedIndirectCount = (AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINDEXEDINSTANCEDINDIRECTCOUNTINDIRECT)GetSharedLibraryFunction(
        *m_AGSLibrary, "agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirectCountIndirect");

    m_AGS.CreateDeviceD3D12 = (AGS_DRIVEREXTENSIONSDX12_CREATEDEVICE)GetSharedLibraryFunction(*m_AGSLibrary, "agsDriverExtensionsDX12_CreateDevice");
    m_AGS.DestroyDeviceD3D12 = (AGS_DRIVEREXTENSIONSDX12_DESTROYDEVICE)GetSharedLibraryFunction(*m_AGSLibrary, "agsDriverExtensionsDX12_DestroyDevice");

    const void** functionArray = (const void**)&m_AGS;
    const size_t functionArraySize = sizeof(AGSFunctionTable) / sizeof(void*);
    size_t i = 0;
    for (; i < functionArraySize && functionArray[i] != nullptr; i++)
        ;

    if (i != functionArraySize) {
        REPORT_WARNING(m_DeviceBase, "AMDAGS is disabled, because not all functions are found in the DLL");
        UnloadSharedLibrary(*m_AGSLibrary);
        m_AGSLibrary = nullptr;

        return;
    }

    AGSGPUInfo gpuInfo = {};
    AGSConfiguration config = {};
    if (!agsContext) {
        const AGSReturnCode result = m_AGS.Initialize(AGS_CURRENT_VERSION, &config, &agsContext, &gpuInfo);
        if (result != AGS_SUCCESS || !agsContext) {
            REPORT_ERROR(m_DeviceBase, "Failed to initialize AMDAGS: %d", (int32_t)result);
            UnloadSharedLibrary(*m_AGSLibrary);
            m_AGSLibrary = nullptr;
        }
    }

    m_AGSContext = agsContext;
}

// D3D11
#if defined(__d3d11_h__)
void Ext::BeginUAVOverlap(ID3D11DeviceContext* deviceContext) const {
    if (m_IsNvAPIAvailable) {
        const NvAPI_Status res = NvAPI_D3D11_BeginUAVOverlap(deviceContext);
        RETURN_ON_FAILURE(m_DeviceBase, res == NVAPI_OK, ReturnVoid(), "NvAPI_D3D11_BeginUAVOverlap() - FAILED!");
    } else if (m_AGSContext) {
        const AGSReturnCode res = m_AGS.BeginUAVOverlap(m_AGSContext, deviceContext);
        RETURN_ON_FAILURE(m_DeviceBase, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_BeginUAVOverlap() - FAILED!");
    }
}

void Ext::EndUAVOverlap(ID3D11DeviceContext* deviceContext) const {
    if (m_IsNvAPIAvailable) {
        const NvAPI_Status status = NvAPI_D3D11_EndUAVOverlap(deviceContext);
        RETURN_ON_FAILURE(m_DeviceBase, status == NVAPI_OK, ReturnVoid(), "NvAPI_D3D11_EndUAVOverlap() - FAILED!");
    } else if (m_AGSContext) {
        const AGSReturnCode res = m_AGS.EndUAVOverlap(m_AGSContext, deviceContext);
        RETURN_ON_FAILURE(m_DeviceBase, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_EndUAVOverlap() - FAILED!");
    }
}

void Ext::WaitForDrain(ID3D11DeviceContext* deviceContext, uint32_t flags) const {
    if (m_IsNvAPIAvailable) {
        const NvAPI_Status res = NvAPI_D3D11_BeginUAVOverlapEx(deviceContext, flags);
        RETURN_ON_FAILURE(m_DeviceBase, res == NVAPI_OK, ReturnVoid(), "NvAPI_D3D11_BeginUAVOverlap() - FAILED!");
    } else if (m_AGSContext) {
        // TODO: verify that this code actually works on AMD!
        const AGSReturnCode res1 = m_AGS.EndUAVOverlap(m_AGSContext, deviceContext);
        RETURN_ON_FAILURE(m_DeviceBase, res1 == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_EndUAVOverlap() - FAILED!");
        const AGSReturnCode res2 = m_AGS.BeginUAVOverlap(m_AGSContext, deviceContext);
        RETURN_ON_FAILURE(m_DeviceBase, res2 == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_BeginUAVOverlap() - FAILED!");
    }
}

void Ext::SetDepthBounds(ID3D11DeviceContext* deviceContext, float minBound, float maxBound) const {
    bool isEnabled = minBound != 0.0f || maxBound != 1.0f;

    if (m_IsNvAPIAvailable) {
        const NvAPI_Status status = NvAPI_D3D11_SetDepthBoundsTest(deviceContext, isEnabled, minBound, maxBound);
        RETURN_ON_FAILURE(m_DeviceBase, status == NVAPI_OK, ReturnVoid(), "NvAPI_D3D11_SetDepthBoundsTest() - FAILED!");
    } else if (m_AGSContext) {
        const AGSReturnCode res = m_AGS.SetDepthBounds(m_AGSContext, deviceContext, isEnabled, minBound, maxBound);
        RETURN_ON_FAILURE(m_DeviceBase, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_SetDepthBounds() - FAILED!");
    }
}

void Ext::DrawIndirect(
    ID3D11DeviceContext* deviceContext, ID3D11Buffer* buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, ID3D11Buffer* countBuffer, uint32_t countBufferOffset) const {
    if (countBuffer) {
        if (m_AGSContext) {
            const AGSReturnCode res = m_AGS.DrawIndirectCount(m_AGSContext, deviceContext, countBuffer, countBufferOffset, buffer, (uint32_t)offset, stride);
            RETURN_ON_FAILURE(m_DeviceBase, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_MultiDrawInstancedIndirectCountIndirect() - FAILED!");
        }
    } else {
        if (m_IsNvAPIAvailable && drawNum > 1) {
            const NvAPI_Status status = NvAPI_D3D11_MultiDrawInstancedIndirect(deviceContext, drawNum, buffer, (uint32_t)offset, stride);
            RETURN_ON_FAILURE(m_DeviceBase, status == NVAPI_OK, ReturnVoid(), "NvAPI_D3D11_MultiDrawInstancedIndirect() - FAILED!");
        } else if (m_AGSContext && drawNum > 1) {
            const AGSReturnCode res = m_AGS.DrawIndirect(m_AGSContext, deviceContext, drawNum, buffer, (uint32_t)offset, stride);
            RETURN_ON_FAILURE(m_DeviceBase, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_MultiDrawInstancedIndirect() - FAILED!");
        } else {
            for (uint32_t i = 0; i < drawNum; i++) {
                deviceContext->DrawInstancedIndirect(buffer, (uint32_t)offset);
                offset += stride;
            }
        }
    }
}

void Ext::DrawIndexedIndirect(
    ID3D11DeviceContext* deviceContext, ID3D11Buffer* buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, ID3D11Buffer* countBuffer, uint32_t countBufferOffset) const {
    if (countBuffer) {
        if (m_AGSContext) {
            const AGSReturnCode res = m_AGS.DrawIndexedIndirectCount(m_AGSContext, deviceContext, countBuffer, countBufferOffset, buffer, (uint32_t)offset, stride);
            RETURN_ON_FAILURE(m_DeviceBase, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirectCountIndirect() - FAILED!");
        }
    } else {
        if (m_IsNvAPIAvailable && drawNum > 1) {
            const NvAPI_Status status = NvAPI_D3D11_MultiDrawIndexedInstancedIndirect(deviceContext, drawNum, buffer, (uint32_t)offset, stride);
            RETURN_ON_FAILURE(m_DeviceBase, status == NVAPI_OK, ReturnVoid(), "NvAPI_D3D11_MultiDrawIndexedInstancedIndirect() - FAILED!");
        } else if (m_AGSContext && drawNum > 1) {
            const AGSReturnCode res = m_AGS.DrawIndexedIndirect(m_AGSContext, deviceContext, drawNum, buffer, (uint32_t)offset, stride);
            RETURN_ON_FAILURE(m_DeviceBase, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect() - FAILED!");
        } else {
            for (uint32_t i = 0; i < drawNum; i++) {
                deviceContext->DrawIndexedInstancedIndirect(buffer, (uint32_t)offset);
                offset += stride;
            }
        }
    }
}
#endif
