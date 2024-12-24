// © 2021 NVIDIA Corporation

#if !defined(__d3d11_h__)

void Ext::InitializePixExt() {
    // Load library
    m_PixLibrary = LoadSharedLibrary("WinPixEventRuntime.dll");
    if (!m_PixLibrary)
        return;

    // Get functions
    m_Pix.BeginEventOnCommandList = (PIX_BEGINEVENTONCOMMANDLIST)GetSharedLibraryFunction(*m_PixLibrary, "PIXBeginEventOnCommandList");
    m_Pix.EndEventOnCommandList = (PIX_ENDEVENTONCOMMANDLIST)GetSharedLibraryFunction(*m_PixLibrary, "PIXEndEventOnCommandList");
    m_Pix.SetMarkerOnCommandList = (PIX_SETMARKERONCOMMANDLIST)GetSharedLibraryFunction(*m_PixLibrary, "PIXSetMarkerOnCommandList");

    // Verify
    const void** functionArray = (const void**)&m_Pix;
    const size_t functionArraySize = sizeof(m_Pix) / sizeof(void*);
    size_t i = 0;
    for (; i < functionArraySize && functionArray[i] != nullptr; i++)
        ;

    if (i != functionArraySize) {
        REPORT_WARNING(m_DeviceBase, "PIX is disabled, because not all functions are found in the DLL");
        UnloadSharedLibrary(*m_PixLibrary);
        m_PixLibrary = nullptr;
    }
}

#endif

#if NRI_USE_EXT_LIBS

Ext::~Ext() {
    if (m_IsNvapiAvailable)
        NvAPI_Unload();

    if (m_AgsContext) {
        if (!m_IsImported)
            m_Ags.Deinitialize(m_AgsContext);
        m_AgsContext = nullptr;

        UnloadSharedLibrary(*m_AgsLibrary);
        m_AgsLibrary = nullptr;
    }

#    if !defined(__d3d11_h__)
    if (m_PixLibrary) {
        UnloadSharedLibrary(*m_PixLibrary);
        m_PixLibrary = nullptr;
    }
#    endif
}

void Ext::InitializeNvExt(const nri::DeviceBase* deviceBase, bool isNVAPILoadedInApp, bool isImported) {
    if (GetModuleHandleA("renderdoc.dll") != nullptr) {
        REPORT_WARNING(deviceBase, "NVAPI is disabled, because RenderDoc library has been loaded");
        return;
    }

    m_DeviceBase = deviceBase;
    m_IsImported = isImported;

    if (isImported && !isNVAPILoadedInApp)
        REPORT_WARNING(m_DeviceBase, "NVAPI is disabled, because it's not loaded on the application side");
    else {
        const NvAPI_Status status = NvAPI_Initialize();
        m_IsNvapiAvailable = (status == NVAPI_OK);
        if (!m_IsNvapiAvailable)
            REPORT_ERROR(m_DeviceBase, "Failed to initialize NVAPI: %d", (int32_t)status);
    }
}

void Ext::InitializeAmdExt(const nri::DeviceBase* deviceBase, AGSContext* agsContext, bool isImported) {
    m_DeviceBase = deviceBase;
    m_IsImported = isImported;

    if (isImported && !agsContext) {
        REPORT_WARNING(m_DeviceBase, "AMDAGS is disabled, because 'agsContext' is not provided");
        return;
    }

    // Load library
    m_AgsLibrary = LoadSharedLibrary("amd_ags_x64.dll");
    if (!m_AgsLibrary) {
        REPORT_WARNING(m_DeviceBase, "AMDAGS is disabled, because 'amd_ags_x64' is not found");
        return;
    }

    // Get functions
    m_Ags.Initialize = (AGS_INITIALIZE)GetSharedLibraryFunction(*m_AgsLibrary, "agsInitialize");
    m_Ags.Deinitialize = (AGS_DEINITIALIZE)GetSharedLibraryFunction(*m_AgsLibrary, "agsDeInitialize");

#    if defined(__d3d11_h__)

    m_Ags.CreateDeviceD3D11 = (AGS_DRIVEREXTENSIONSDX11_CREATEDEVICE)GetSharedLibraryFunction(*m_AgsLibrary, "agsDriverExtensionsDX11_CreateDevice");
    m_Ags.DestroyDeviceD3D11 = (AGS_DRIVEREXTENSIONSDX11_DESTROYDEVICE)GetSharedLibraryFunction(*m_AgsLibrary, "agsDriverExtensionsDX11_DestroyDevice");
    m_Ags.BeginUAVOverlap = (AGS_DRIVEREXTENSIONSDX11_BEGINUAVOVERLAP)GetSharedLibraryFunction(*m_AgsLibrary, "agsDriverExtensionsDX11_BeginUAVOverlap");
    m_Ags.EndUAVOverlap = (AGS_DRIVEREXTENSIONSDX11_ENDUAVOVERLAP)GetSharedLibraryFunction(*m_AgsLibrary, "agsDriverExtensionsDX11_EndUAVOverlap");
    m_Ags.SetDepthBounds = (AGS_DRIVEREXTENSIONSDX11_SETDEPTHBOUNDS)GetSharedLibraryFunction(*m_AgsLibrary, "agsDriverExtensionsDX11_SetDepthBounds");
    m_Ags.DrawIndirect = (AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINSTANCEDINDIRECT)GetSharedLibraryFunction(*m_AgsLibrary, "agsDriverExtensionsDX11_MultiDrawInstancedIndirect");
    m_Ags.DrawIndexedIndirect = (AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINDEXEDINSTANCEDINDIRECT)GetSharedLibraryFunction(*m_AgsLibrary, "agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect");
    m_Ags.DrawIndirectCount = (AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINSTANCEDINDIRECTCOUNTINDIRECT)GetSharedLibraryFunction(*m_AgsLibrary, "agsDriverExtensionsDX11_MultiDrawInstancedIndirectCountIndirect");
    m_Ags.DrawIndexedIndirectCount = (AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINDEXEDINSTANCEDINDIRECTCOUNTINDIRECT)GetSharedLibraryFunction(*m_AgsLibrary, "agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirectCountIndirect");

#    else

    m_Ags.CreateDeviceD3D12 = (AGS_DRIVEREXTENSIONSDX12_CREATEDEVICE)GetSharedLibraryFunction(*m_AgsLibrary, "agsDriverExtensionsDX12_CreateDevice");
    m_Ags.DestroyDeviceD3D12 = (AGS_DRIVEREXTENSIONSDX12_DESTROYDEVICE)GetSharedLibraryFunction(*m_AgsLibrary, "agsDriverExtensionsDX12_DestroyDevice");

#    endif

    // Verify
    const void** functionArray = (const void**)&m_Ags;
    const size_t functionArraySize = sizeof(m_Ags) / sizeof(void*);
    size_t i = 0;
    for (; i < functionArraySize && functionArray[i] != nullptr; i++)
        ;

    if (i != functionArraySize) {
        REPORT_WARNING(m_DeviceBase, "AMDAGS is disabled, because not all functions are found in the DLL");
        UnloadSharedLibrary(*m_AgsLibrary);
        m_AgsLibrary = nullptr;

        return;
    }

    // Initialize
    AGSGPUInfo gpuInfo = {};
    AGSConfiguration config = {};
    if (!agsContext) {
        const AGSReturnCode result = m_Ags.Initialize(AGS_CURRENT_VERSION, &config, &agsContext, &gpuInfo);
        if (result != AGS_SUCCESS || !agsContext) {
            REPORT_ERROR(m_DeviceBase, "Failed to initialize AMDAGS: %d", (int32_t)result);
            UnloadSharedLibrary(*m_AgsLibrary);
            m_AgsLibrary = nullptr;
        }
    }

    m_AgsContext = agsContext;
}

#    if defined(__d3d11_h__)

void Ext::BeginUAVOverlap(ID3D11DeviceContext* deviceContext) const {
    if (m_IsNvapiAvailable) {
        const NvAPI_Status res = NvAPI_D3D11_BeginUAVOverlap(deviceContext);
        RETURN_ON_FAILURE(m_DeviceBase, res == NVAPI_OK, ReturnVoid(), "NvAPI_D3D11_BeginUAVOverlap()  failed!");
    } else if (m_AgsContext) {
        const AGSReturnCode res = m_Ags.BeginUAVOverlap(m_AgsContext, deviceContext);
        RETURN_ON_FAILURE(m_DeviceBase, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_BeginUAVOverlap()  failed!");
    }
}

void Ext::EndUAVOverlap(ID3D11DeviceContext* deviceContext) const {
    if (m_IsNvapiAvailable) {
        const NvAPI_Status status = NvAPI_D3D11_EndUAVOverlap(deviceContext);
        RETURN_ON_FAILURE(m_DeviceBase, status == NVAPI_OK, ReturnVoid(), "NvAPI_D3D11_EndUAVOverlap()  failed!");
    } else if (m_AgsContext) {
        const AGSReturnCode res = m_Ags.EndUAVOverlap(m_AgsContext, deviceContext);
        RETURN_ON_FAILURE(m_DeviceBase, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_EndUAVOverlap()  failed!");
    }
}

void Ext::WaitForDrain(ID3D11DeviceContext* deviceContext, uint32_t flags) const {
    if (m_IsNvapiAvailable) {
        const NvAPI_Status res = NvAPI_D3D11_BeginUAVOverlapEx(deviceContext, flags);
        RETURN_ON_FAILURE(m_DeviceBase, res == NVAPI_OK, ReturnVoid(), "NvAPI_D3D11_BeginUAVOverlap()  failed!");
    } else if (m_AgsContext) {
        // TODO: verify that this code actually works on AMD!
        const AGSReturnCode res1 = m_Ags.EndUAVOverlap(m_AgsContext, deviceContext);
        RETURN_ON_FAILURE(m_DeviceBase, res1 == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_EndUAVOverlap()  failed!");
        const AGSReturnCode res2 = m_Ags.BeginUAVOverlap(m_AgsContext, deviceContext);
        RETURN_ON_FAILURE(m_DeviceBase, res2 == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_BeginUAVOverlap()  failed!");
    }
}

void Ext::SetDepthBounds(ID3D11DeviceContext* deviceContext, float minBound, float maxBound) const {
    bool isEnabled = minBound != 0.0f || maxBound != 1.0f;

    if (m_IsNvapiAvailable) {
        const NvAPI_Status status = NvAPI_D3D11_SetDepthBoundsTest(deviceContext, isEnabled, minBound, maxBound);
        RETURN_ON_FAILURE(m_DeviceBase, status == NVAPI_OK, ReturnVoid(), "NvAPI_D3D11_SetDepthBoundsTest()  failed!");
    } else if (m_AgsContext) {
        const AGSReturnCode res = m_Ags.SetDepthBounds(m_AgsContext, deviceContext, isEnabled, minBound, maxBound);
        RETURN_ON_FAILURE(m_DeviceBase, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_SetDepthBounds()  failed!");
    }
}

void Ext::DrawIndirect(ID3D11DeviceContext* deviceContext, ID3D11Buffer* buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, ID3D11Buffer* countBuffer, uint32_t countBufferOffset) const {
    if (countBuffer) {
        if (m_AgsContext) {
            const AGSReturnCode res = m_Ags.DrawIndirectCount(m_AgsContext, deviceContext, countBuffer, countBufferOffset, buffer, (uint32_t)offset, stride);
            RETURN_ON_FAILURE(m_DeviceBase, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_MultiDrawInstancedIndirectCountIndirect()  failed!");
        }
    } else {
        if (m_IsNvapiAvailable && drawNum > 1) {
            const NvAPI_Status status = NvAPI_D3D11_MultiDrawInstancedIndirect(deviceContext, drawNum, buffer, (uint32_t)offset, stride);
            RETURN_ON_FAILURE(m_DeviceBase, status == NVAPI_OK, ReturnVoid(), "NvAPI_D3D11_MultiDrawInstancedIndirect()  failed!");
        } else if (m_AgsContext && drawNum > 1) {
            const AGSReturnCode res = m_Ags.DrawIndirect(m_AgsContext, deviceContext, drawNum, buffer, (uint32_t)offset, stride);
            RETURN_ON_FAILURE(m_DeviceBase, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_MultiDrawInstancedIndirect()  failed!");
        } else {
            for (uint32_t i = 0; i < drawNum; i++) {
                deviceContext->DrawInstancedIndirect(buffer, (uint32_t)offset);
                offset += stride;
            }
        }
    }
}

void Ext::DrawIndexedIndirect(ID3D11DeviceContext* deviceContext, ID3D11Buffer* buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, ID3D11Buffer* countBuffer, uint32_t countBufferOffset) const {
    if (countBuffer) {
        if (m_AgsContext) {
            const AGSReturnCode res = m_Ags.DrawIndexedIndirectCount(m_AgsContext, deviceContext, countBuffer, countBufferOffset, buffer, (uint32_t)offset, stride);
            RETURN_ON_FAILURE(m_DeviceBase, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirectCountIndirect()  failed!");
        }
    } else {
        if (m_IsNvapiAvailable && drawNum > 1) {
            const NvAPI_Status status = NvAPI_D3D11_MultiDrawIndexedInstancedIndirect(deviceContext, drawNum, buffer, (uint32_t)offset, stride);
            RETURN_ON_FAILURE(m_DeviceBase, status == NVAPI_OK, ReturnVoid(), "NvAPI_D3D11_MultiDrawIndexedInstancedIndirect()  failed!");
        } else if (m_AgsContext && drawNum > 1) {
            const AGSReturnCode res = m_Ags.DrawIndexedIndirect(m_AgsContext, deviceContext, drawNum, buffer, (uint32_t)offset, stride);
            RETURN_ON_FAILURE(m_DeviceBase, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect()  failed!");
        } else {
            for (uint32_t i = 0; i < drawNum; i++) {
                deviceContext->DrawIndexedInstancedIndirect(buffer, (uint32_t)offset);
                offset += stride;
            }
        }
    }
}

#    endif

#else

#    if defined(__d3d11_h__)

void Ext::DrawIndirect(ID3D11DeviceContext* deviceContext, ID3D11Buffer* buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, ID3D11Buffer* countBuffer, uint32_t) const {
    if (!countBuffer) {
        for (uint32_t i = 0; i < drawNum; i++) {
            deviceContext->DrawInstancedIndirect(buffer, (uint32_t)offset);
            offset += stride;
        }
    }
}

void Ext::DrawIndexedIndirect(ID3D11DeviceContext* deviceContext, ID3D11Buffer* buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, ID3D11Buffer* countBuffer, uint32_t) const {
    if (!countBuffer) {
        for (uint32_t i = 0; i < drawNum; i++) {
            deviceContext->DrawIndexedInstancedIndirect(buffer, (uint32_t)offset);
            offset += stride;
        }
    }
}

#    else

Ext::~Ext() {
    if (m_PixLibrary) {
        UnloadSharedLibrary(*m_PixLibrary);
        m_PixLibrary = nullptr;
    }
}

#    endif

#endif
