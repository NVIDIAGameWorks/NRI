// © 2021 NVIDIA Corporation

#pragma once

struct ID3D11Device5;
typedef ID3D11Device5 ID3D11DeviceBest;

namespace nri {

struct CommandQueueD3D11;

struct DeviceD3D11 final : public DeviceBase {
    DeviceD3D11(const CallbackInterface& callbacks, StdAllocator<uint8_t>& stdAllocator);
    ~DeviceD3D11();

    inline ID3D11DeviceBest* GetNativeObject() const {
        return m_Device.GetInterface();
    }

    inline ID3D11DeviceBest* operator->() const {
        return m_Device;
    }

    inline uint8_t GetVersion() const {
        return m_Version;
    }

    inline const d3d11::Ext* GetExt() const {
        return &m_Ext;
    }

    inline IDXGIAdapter* GetAdapter() const {
        return m_Adapter;
    }

    inline ID3D11DeviceContextBest* GetImmediateContext() {
        return m_ImmediateContext;
    }

    inline uint8_t GetImmediateContextVersion() {
        return m_ImmediateContextVersion;
    }

    inline const CoreInterface& GetCoreInterface() const {
        return m_CoreInterface;
    }

    inline bool IsDeferredContextEmulated() const {
        return m_IsDeferredContextEmulated;
    }

    inline void EnterCriticalSection() {
        if (m_Multithread)
            m_Multithread->Enter();
        else
            ::EnterCriticalSection(&m_CriticalSection);
    }

    inline void LeaveCriticalSection() {
        if (m_Multithread)
            m_Multithread->Leave();
        else
            ::LeaveCriticalSection(&m_CriticalSection);
    }

    template <typename Implementation, typename Interface, typename... Args>
    inline Result CreateImplementation(Interface*& entity, const Args&... args) {
        Implementation* impl = Allocate<Implementation>(GetStdAllocator(), *this);
        Result result = impl->Create(args...);

        if (result != Result::SUCCESS) {
            Destroy(GetStdAllocator(), impl);
            entity = nullptr;
        } else
            entity = (Interface*)impl;

        return result;
    }

    Result Create(const DeviceCreationDesc& deviceCreationDesc, ID3D11Device* precreatedDevice, AGSContext* agsContext, bool isNVAPILoadedInApp);
    void GetMemoryDesc(const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;
    void GetMemoryDesc(const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator);
    Result GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue);
    Result BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    FormatSupportBits GetFormatSupport(Format format) const;

    inline void SetDebugName(const char* name) {
        SET_D3D_DEBUG_OBJECT_NAME(m_Device, name);
        SET_D3D_DEBUG_OBJECT_NAME(m_ImmediateContext, name);
    }

    //================================================================================================================
    // DeviceBase
    //================================================================================================================

    const DeviceDesc& GetDesc() const {
        return m_Desc;
    }

    void Destruct();
    Result FillFunctionTable(CoreInterface& table) const;
    Result FillFunctionTable(HelperInterface& table) const;
    Result FillFunctionTable(LowLatencyInterface& table) const;
    Result FillFunctionTable(StreamerInterface& table) const;
    Result FillFunctionTable(SwapChainInterface& table) const;
    Result FillFunctionTable(ResourceAllocatorInterface& table) const;
    Result FillFunctionTable(WrapperD3D11Interface& table) const;

private:
    void FillDesc();

private:
    d3d11::Ext m_Ext = {}; // don't sort: destructor must be called last!
    ComPtr<ID3D11DeviceBest> m_Device;
    ComPtr<IDXGIAdapter> m_Adapter;
    ComPtr<ID3D11DeviceContextBest> m_ImmediateContext;
    ComPtr<ID3D11Multithread> m_Multithread;
    std::array<CommandQueueD3D11*, (size_t)CommandQueueType::MAX_NUM> m_CommandQueues = {};
    DeviceDesc m_Desc = {};
    CRITICAL_SECTION m_CriticalSection = {}; // TODO: Lock?
    CoreInterface m_CoreInterface = {};
    uint8_t m_Version = 0;
    uint8_t m_ImmediateContextVersion = 0;
    bool m_IsWrapped = false;
    bool m_IsDeferredContextEmulated = false;
};

} // namespace nri
