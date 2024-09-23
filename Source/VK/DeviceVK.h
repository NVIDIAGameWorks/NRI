// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct CommandQueueVK;

struct IsSupported {
    uint32_t descriptorIndexing : 1;
    uint32_t deviceAddress : 1;
    uint32_t swapChainMutableFormat : 1;
    uint32_t presentId : 1;
    uint32_t presentWait : 1;
    uint32_t lowLatency : 1;
    uint32_t memoryPriority : 1;
    uint32_t memoryBudget : 1;
    uint32_t maintenance5 : 1;
    uint32_t imageSlicedView : 1;
    uint32_t customBorderColor : 1;
};

struct DeviceVK final : public DeviceBase {
    inline operator VkDevice() const {
        return m_Device;
    }

    inline operator VkPhysicalDevice() const {
        return m_PhysicalDevice;
    }

    inline operator VkInstance() const {
        return m_Instance;
    }

    inline const DispatchTable& GetDispatchTable() const {
        return m_VK;
    }

    inline const VkAllocationCallbacks* GetAllocationCallbacks() const {
        return m_AllocationCallbackPtr;
    }

    inline const SPIRVBindingOffsets& GetSPIRVBindingOffsets() const {
        return m_SPIRVBindingOffsets;
    }

    inline const CoreInterface& GetCoreInterface() const {
        return m_CoreInterface;
    }

    inline bool IsHostCoherentMemory(MemoryTypeIndex memoryTypeIndex) const {
        return (m_MemoryProps.memoryTypes[memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
    }

    inline VmaAllocator_T* GetVma() const {
        return m_Vma;
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

    DeviceVK(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator);
    ~DeviceVK();

    Result Create(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationVKDesc& deviceCreationVKDesc, bool isWrapper);
    void FillCreateInfo(const BufferDesc& bufferDesc, VkBufferCreateInfo& info) const;
    void FillCreateInfo(const TextureDesc& bufferDesc, VkImageCreateInfo& info) const;
    void GetMemoryDesc(const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;
    void GetMemoryDesc(const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;
    void GetMemoryDesc(const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc);
    bool GetMemoryTypeInfo(MemoryLocation memoryLocation, uint32_t memoryTypeMask, MemoryTypeInfo& memoryTypeInfo) const;
    bool GetMemoryTypeByIndex(uint32_t index, MemoryTypeInfo& memoryTypeInfo) const;
    void GetAccelerationStructureBuildSizesInfo(const AccelerationStructureDesc& accelerationStructureDesc, VkAccelerationStructureBuildSizesInfoKHR& sizesInfo);
    void SetDebugNameToTrivialObject(VkObjectType objectType, uint64_t handle, const char* name);
    Result CreateVma();
    void DestroyVma();

    //================================================================================================================
    // DeviceBase
    //================================================================================================================

    void Destruct();
    Result FillFunctionTable(CoreInterface& table) const;
    Result FillFunctionTable(HelperInterface& table) const;
    Result FillFunctionTable(LowLatencyInterface& table) const;
    Result FillFunctionTable(MeshShaderInterface& table) const;
    Result FillFunctionTable(RayTracingInterface& table) const;
    Result FillFunctionTable(StreamerInterface& table) const;
    Result FillFunctionTable(SwapChainInterface& table) const;
    Result FillFunctionTable(ResourceAllocatorInterface& table) const;
    Result FillFunctionTable(WrapperVKInterface& table) const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    Result CreateCommandQueue(const CommandQueueVKDesc& commandQueueDesc, CommandQueue*& commandQueue);
    Result GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue);
    Result BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result QueryVideoMemoryInfo(MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) const;
    Result BindAccelerationStructureMemory(const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    FormatSupportBits GetFormatSupport(Format format) const;

private:
    void FilterInstanceLayers(Vector<const char*>& layers);
    void ProcessInstanceExtensions(Vector<const char*>& desiredInstanceExts);
    void ProcessDeviceExtensions(Vector<const char*>& desiredDeviceExts, bool disableRayTracing);
    void FillFamilyIndices(bool isWrapper, const DeviceCreationVKDesc& deviceCreationVKDesc);
    void ReportDeviceGroupInfo();
    void GetAdapterDesc();
    Result CreateInstance(bool enableGraphicsAPIValidation, const Vector<const char*>& desiredInstanceExts);
    Result ResolvePreInstanceDispatchTable();
    Result ResolveInstanceDispatchTable(const Vector<const char*>& desiredInstanceExts);
    Result ResolveDispatchTable(const Vector<const char*>& desiredDeviceExts);

public:
    union {
        uint32_t m_IsSupportedStorage = 0;
        IsSupported m_IsSupported;
    };

private:
    VkPhysicalDevice m_PhysicalDevice = nullptr;
    std::array<uint32_t, (uint32_t)CommandQueueType::MAX_NUM> m_ActiveQueueFamilyIndices = {};
    std::array<uint32_t, (uint32_t)CommandQueueType::MAX_NUM> m_QueueFamilyIndices = {};
    std::array<CommandQueueVK*, (uint32_t)CommandQueueType::MAX_NUM> m_CommandQueues = {};
    DispatchTable m_VK = {};
    VkPhysicalDeviceMemoryProperties m_MemoryProps = {};
    VkAllocationCallbacks m_AllocationCallbacks = {};
    SPIRVBindingOffsets m_SPIRVBindingOffsets = {};
    CoreInterface m_CoreInterface = {};
    Library* m_Loader = nullptr;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkAllocationCallbacks* m_AllocationCallbackPtr = nullptr;
    VkDebugUtilsMessengerEXT m_Messenger = VK_NULL_HANDLE;
    VmaAllocator_T* m_Vma = nullptr;
    uint32_t m_NumActiveFamilyIndices = 0;
    uint32_t m_MinorVersion = 0;
    bool m_OwnsNativeObjects = true;
    Lock m_Lock;
};

} // namespace nri
