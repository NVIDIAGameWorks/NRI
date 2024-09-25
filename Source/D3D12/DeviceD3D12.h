// © 2021 NVIDIA Corporation

#pragma once

struct IDXGIAdapter;
struct ID3D12DescriptorHeap;
struct ID3D12CommandSignature;
struct D3D12_CPU_DESCRIPTOR_HANDLE;

#ifdef NRI_USE_AGILITY_SDK
struct ID3D12Device14;
typedef ID3D12Device14 ID3D12DeviceBest;
#else
struct ID3D12Device5;
typedef ID3D12Device5 ID3D12DeviceBest;
#endif

namespace nri {

struct CommandQueueD3D12;

constexpr size_t DESCRIPTOR_HEAP_TYPE_NUM = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
constexpr uint32_t DESCRIPTORS_BATCH_SIZE = 1024;

struct DeviceD3D12 final : public DeviceBase {
    DeviceD3D12(const CallbackInterface& callbacks, StdAllocator<uint8_t>& stdAllocator);
    ~DeviceD3D12();

    inline ID3D12DeviceBest* GetNativeObject() const {
        return m_Device;
    }

    inline ID3D12DeviceBest* operator->() const {
        return m_Device;
    }

    inline uint8_t GetVersion() const {
        return m_Version;
    }

    inline const d3d12::Ext* GetExt() const {
        return &m_Ext;
    }

    inline IDXGIAdapter* GetAdapter() const {
        return m_Adapter;
    }

    inline const CoreInterface& GetCoreInterface() const {
        return m_CoreInterface;
    }

    inline D3D12MA::Allocator* GetVma() const {
        return m_Vma;
    }

    inline void FreeDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, const DescriptorHandle& descriptorHandle) {
        ExclusiveScope lock(m_FreeDescriptorLocks[type]);
        auto& freeDescriptors = m_FreeDescriptors[type];
        freeDescriptors.push_back(descriptorHandle);
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

    Result Create(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationD3D12Desc& deviceCreationD3D12Desc);
    Result CreateDefaultDrawSignatures(ID3D12RootSignature* rootSignature, bool enableDrawParametersEmulation);
    Result CreateCpuOnlyVisibleDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);
    Result GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, DescriptorHandle& descriptorHandle);
    DescriptorPointerCPU GetDescriptorPointerCPU(const DescriptorHandle& descriptorHandle);
    void GetMemoryDesc(MemoryLocation memoryLocation, const D3D12_RESOURCE_DESC& resourceDesc, MemoryDesc& memoryDesc) const;
    void GetAccelerationStructureMemoryDesc(const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc);
    void GetAccelerationStructurePrebuildInfo(const AccelerationStructureDesc& accelerationStructureDesc, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& prebuildInfo);
    ID3D12CommandSignature* GetDrawCommandSignature(uint32_t stride, ID3D12RootSignature* rootSignature);
    ID3D12CommandSignature* GetDrawIndexedCommandSignature(uint32_t stride, ID3D12RootSignature* rootSignature);
    ID3D12CommandSignature* GetDrawMeshCommandSignature(uint32_t stride);
    ID3D12CommandSignature* GetDispatchRaysCommandSignature() const;
    ID3D12CommandSignature* GetDispatchCommandSignature() const;
    Result CreateVma();

    //================================================================================================================
    // DeviceBase
    //================================================================================================================

    inline const DeviceDesc& GetDesc() const {
        return m_Desc;
    }

    void Destruct();
    Result FillFunctionTable(CoreInterface& table) const;
    Result FillFunctionTable(HelperInterface& table) const;
    Result FillFunctionTable(LowLatencyInterface& table) const;
    Result FillFunctionTable(MeshShaderInterface& table) const;
    Result FillFunctionTable(RayTracingInterface& table) const;
    Result FillFunctionTable(StreamerInterface& table) const;
    Result FillFunctionTable(SwapChainInterface& table) const;
    Result FillFunctionTable(ResourceAllocatorInterface& table) const;
    Result FillFunctionTable(WrapperD3D12Interface& table) const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result CreateCommandQueue(void* d3d12commandQueue, CommandQueueD3D12*& commandQueue);
    Result GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue);
    Result BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindAccelerationStructureMemory(const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    FormatSupportBits GetFormatSupport(Format format) const;

    inline void SetDebugName(const char* name) {
        SET_D3D_DEBUG_OBJECT_NAME(m_Device, name);
    }

private:
    void FillDesc(const DeviceCreationDesc& deviceCreationDesc);
    ComPtr<ID3D12CommandSignature> CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE type, uint32_t stride, ID3D12RootSignature* rootSignature, bool enableDrawParametersEmulation = false);

private:
    d3d12::Ext m_Ext = {}; // don't sort: destructor must be called last!
    ComPtr<ID3D12DeviceBest> m_Device;
    ComPtr<IDXGIAdapter> m_Adapter;
    ComPtr<ID3D12CommandSignature> m_DispatchCommandSignature;
    ComPtr<ID3D12CommandSignature> m_DispatchRaysCommandSignature;
    ComPtr<D3D12MA::Allocator> m_Vma;
    Vector<DescriptorHeapDesc> m_DescriptorHeaps;
    Vector<Vector<DescriptorHandle>> m_FreeDescriptors;
    UnorderedMap<uint64_t, ComPtr<ID3D12CommandSignature>> m_DrawCommandSignatures;
    UnorderedMap<uint64_t, ComPtr<ID3D12CommandSignature>> m_DrawIndexedCommandSignatures;
    UnorderedMap<uint32_t, ComPtr<ID3D12CommandSignature>> m_DrawMeshCommandSignatures;
    std::array<CommandQueueD3D12*, (size_t)CommandQueueType::MAX_NUM> m_CommandQueues = {};
    D3D12MA::ALLOCATION_CALLBACKS m_AllocationCallbacks = {};
    D3D12MA::ALLOCATION_CALLBACKS* m_AllocationCallbackPtr = nullptr;
    CoreInterface m_CoreInterface = {};
    DeviceDesc m_Desc = {};
    uint8_t m_Version = 0;
    bool m_IsWrapped = false;
    std::array<Lock, DESCRIPTOR_HEAP_TYPE_NUM> m_FreeDescriptorLocks;
    Lock m_DescriptorHeapLock;
    Lock m_QueueLock;
};

} // namespace nri
