#pragma once

// TODO: D3DExt reading requires mental gymnastics...

// PIX events // TODO: currently the DLL doesn't export markers for CPU-only timeline and counters
#if !defined(__d3d11_h__)

typedef HRESULT(WINAPI* PIX_BEGINEVENTONCOMMANDLIST)(ID3D12GraphicsCommandList* commandList, UINT64 color, _In_ PCSTR formatString);
typedef HRESULT(WINAPI* PIX_ENDEVENTONCOMMANDLIST)(ID3D12GraphicsCommandList* commandList);
typedef HRESULT(WINAPI* PIX_SETMARKERONCOMMANDLIST)(ID3D12GraphicsCommandList* commandList, UINT64 color, _In_ PCSTR formatString);

struct PixFuncTable {
    PIX_BEGINEVENTONCOMMANDLIST BeginEventOnCommandList;
    PIX_ENDEVENTONCOMMANDLIST EndEventOnCommandList;
    PIX_SETMARKERONCOMMANDLIST SetMarkerOnCommandList;
};

#endif

#if NRI_USE_EXT_LIBS

// AMD AGS
struct AgsFuncTable {
    AGS_INITIALIZE Initialize;
    AGS_DEINITIALIZE Deinitialize;

#    if defined(__d3d11_h__)

    AGS_DRIVEREXTENSIONSDX11_CREATEDEVICE CreateDeviceD3D11;
    AGS_DRIVEREXTENSIONSDX11_DESTROYDEVICE DestroyDeviceD3D11;
    AGS_DRIVEREXTENSIONSDX11_BEGINUAVOVERLAP BeginUAVOverlap;
    AGS_DRIVEREXTENSIONSDX11_ENDUAVOVERLAP EndUAVOverlap;
    AGS_DRIVEREXTENSIONSDX11_SETDEPTHBOUNDS SetDepthBounds;
    AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINSTANCEDINDIRECT DrawIndirect;
    AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINDEXEDINSTANCEDINDIRECT DrawIndexedIndirect;
    AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINSTANCEDINDIRECTCOUNTINDIRECT DrawIndirectCount;
    AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINDEXEDINSTANCEDINDIRECTCOUNTINDIRECT DrawIndexedIndirectCount;

#else

    AGS_DRIVEREXTENSIONSDX12_CREATEDEVICE CreateDeviceD3D12;
    AGS_DRIVEREXTENSIONSDX12_DESTROYDEVICE DestroyDeviceD3D12;

#    endif
};

struct Ext {
    ~Ext();

    inline bool HasNvapi() const {
        return m_IsNvapiAvailable;
    }

    inline bool HasAgs() const {
        return m_AgsContext != nullptr;
    }

    void InitializeNvExt(const nri::DeviceBase* deviceBase, bool isNVAPILoadedInApp, bool isImported);
    void InitializeAmdExt(const nri::DeviceBase* deviceBase, AGSContext* agsContext, bool isImported);

#    if defined(__d3d11_h__)

    void BeginUAVOverlap(ID3D11DeviceContext* deviceContext) const;
    void EndUAVOverlap(ID3D11DeviceContext* deviceContext) const;
    void WaitForDrain(ID3D11DeviceContext* deviceContext, uint32_t flags) const;
    void SetDepthBounds(ID3D11DeviceContext* deviceContext, float minBound, float maxBound) const;
    void DrawIndirect(ID3D11DeviceContext* deviceContext, ID3D11Buffer* buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, ID3D11Buffer* countBuffer, uint32_t countBufferOffset) const;
    void DrawIndexedIndirect(ID3D11DeviceContext* deviceContext, ID3D11Buffer* buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, ID3D11Buffer* countBuffer, uint32_t countBufferOffset) const;

#    else

    inline bool HasPix() const {
        return m_PixLibrary != nullptr;
    }

    void InitializePixExt();

    PixFuncTable m_Pix = {};
    Library* m_PixLibrary = nullptr;

#    endif

    const nri::DeviceBase* m_DeviceBase = nullptr;
    AGSContext* m_AgsContext = nullptr;
    AgsFuncTable m_Ags = {};
    Library* m_AgsLibrary = nullptr;
    bool m_IsNvapiAvailable = false;
    bool m_IsImported = false;
};

#else

struct Ext {
    inline bool HasNvapi() const {
        return false;
    }

    inline bool HasAgs() const {
        return false;
    }

    inline void InitializeNvExt(const nri::DeviceBase*, bool, bool) {
    }

    inline void InitializeAmdExt(const nri::DeviceBase*, AGSContext*, bool) {
    }

#    if defined(__d3d11_h__)

    inline void BeginUAVOverlap(ID3D11DeviceContext*) const {
    }

    inline void EndUAVOverlap(ID3D11DeviceContext*) const {
    }

    inline void WaitForDrain(ID3D11DeviceContext*, uint32_t) const {
    }

    inline void SetDepthBounds(ID3D11DeviceContext*, float, float) const {
    }

    void DrawIndirect(ID3D11DeviceContext* deviceContext, ID3D11Buffer* buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, ID3D11Buffer* countBuffer, uint32_t countBufferOffset) const;
    void DrawIndexedIndirect(ID3D11DeviceContext* deviceContext, ID3D11Buffer* buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, ID3D11Buffer* countBuffer, uint32_t countBufferOffset) const;

#    else

    ~Ext();

    inline bool HasPix() const {
        return m_PixLibrary != nullptr;
    }

    void InitializePixExt();

    const nri::DeviceBase* m_DeviceBase = nullptr;
    PixFuncTable m_Pix = {};
    Library* m_PixLibrary = nullptr;

#    endif
};

#endif
