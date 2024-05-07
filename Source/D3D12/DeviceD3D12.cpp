// © 2021 NVIDIA Corporation

#include "SharedD3D12.h"

#include "AccelerationStructureD3D12.h"
#include "BufferD3D12.h"
#include "CommandAllocatorD3D12.h"
#include "CommandBufferD3D12.h"
#include "CommandQueueD3D12.h"
#include "DescriptorD3D12.h"
#include "DescriptorPoolD3D12.h"
#include "FenceD3D12.h"
#include "HelperDeviceMemoryAllocator.h"
#include "MemoryD3D12.h"
#include "PipelineD3D12.h"
#include "PipelineLayoutD3D12.h"
#include "QueryPoolD3D12.h"
#include "Streamer.h"
#include "SwapChainD3D12.h"
#include "TextureD3D12.h"

using namespace nri;

static uint8_t QueryLatestDevice(ComPtr<ID3D12DeviceBest>& in, ComPtr<ID3D12DeviceBest>& out) {
    static const IID versions[] = {
#ifdef NRI_USE_AGILITY_SDK
        __uuidof(ID3D12Device14),
        __uuidof(ID3D12Device13),
        __uuidof(ID3D12Device12),
        __uuidof(ID3D12Device11),
        __uuidof(ID3D12Device10),
        __uuidof(ID3D12Device9),
        __uuidof(ID3D12Device8),
        __uuidof(ID3D12Device7),
        __uuidof(ID3D12Device6),
#endif
        __uuidof(ID3D12Device5),
        __uuidof(ID3D12Device4),
        __uuidof(ID3D12Device3),
        __uuidof(ID3D12Device2),
        __uuidof(ID3D12Device1),
        __uuidof(ID3D12Device),
    };
    const uint8_t n = (uint8_t)GetCountOf(versions);

    uint8_t i = 0;
    for (; i < n; i++) {
        HRESULT hr = in->QueryInterface(versions[i], (void**)&out);
        if (SUCCEEDED(hr))
            break;
    }

    return n - i - 1;
}

static inline uint64_t HashRootSignatureAndStride(ID3D12RootSignature* rootSignature, uint32_t stride) {
    assert(stride < 4096);
    return ((uint64_t)stride << 52ull) | ((uint64_t)rootSignature & ((1ull << 52) - 1));
}

Result CreateDeviceD3D12(const DeviceCreationDesc& deviceCreationDesc, DeviceBase*& device) {
    DeviceCreationD3D12Desc deviceCreationD3D12Desc = {};

    StdAllocator<uint8_t> allocator(deviceCreationDesc.memoryAllocatorInterface);
    DeviceD3D12* implementation = Allocate<DeviceD3D12>(allocator, deviceCreationDesc.callbackInterface, allocator);
    Result result = implementation->Create(deviceCreationDesc, deviceCreationD3D12Desc);

    if (result == Result::SUCCESS) {
        device = (DeviceBase*)implementation;
        return Result::SUCCESS;
    }

    Deallocate(allocator, implementation);

    return result;
}

Result CreateDeviceD3D12(const DeviceCreationD3D12Desc& deviceCreationD3D12Desc, DeviceBase*& device) {
    if (!deviceCreationD3D12Desc.d3d12Device)
        return Result::INVALID_ARGUMENT;

    LUID luid = deviceCreationD3D12Desc.d3d12Device->GetAdapterLuid();

    AdapterDesc adapterDesc = {};
    adapterDesc.luid = *(uint64_t*)&luid;

    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.adapterDesc = &adapterDesc;
    deviceCreationDesc.callbackInterface = deviceCreationD3D12Desc.callbackInterface;
    deviceCreationDesc.memoryAllocatorInterface = deviceCreationD3D12Desc.memoryAllocatorInterface;
    deviceCreationDesc.enableD3D12DrawParametersEmulation = deviceCreationD3D12Desc.enableD3D12DrawParametersEmulation;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::D3D12;

    StdAllocator<uint8_t> allocator(deviceCreationD3D12Desc.memoryAllocatorInterface);
    DeviceD3D12* implementation = Allocate<DeviceD3D12>(allocator, deviceCreationD3D12Desc.callbackInterface, allocator);
    Result result = implementation->Create(deviceCreationDesc, deviceCreationD3D12Desc);

    if (result == Result::SUCCESS) {
        device = implementation;
        return Result::SUCCESS;
    }

    Deallocate(allocator, implementation);

    return result;
}

DeviceD3D12::DeviceD3D12(const CallbackInterface& callbacks, StdAllocator<uint8_t>& stdAllocator) :
    DeviceBase(callbacks, stdAllocator),
    m_DescriptorHeaps(GetStdAllocator()),
    m_FreeDescriptors(GetStdAllocator()),
    m_DrawCommandSignatures(GetStdAllocator()),
    m_DrawIndexedCommandSignatures(GetStdAllocator()),
    m_DrawMeshCommandSignatures(GetStdAllocator()) {
    m_FreeDescriptors.resize(DESCRIPTOR_HEAP_TYPE_NUM, Vector<DescriptorHandle>(GetStdAllocator()));
    m_Desc.graphicsAPI = GraphicsAPI::D3D12;
    m_Desc.nriVersionMajor = NRI_VERSION_MAJOR;
    m_Desc.nriVersionMinor = NRI_VERSION_MINOR;
}

DeviceD3D12::~DeviceD3D12() {
    for (auto& commandQueueD3D12 : m_CommandQueues) {
        if (commandQueueD3D12)
            Deallocate(GetStdAllocator(), commandQueueD3D12);
    }

    if (m_Ext.HasAGS() && !m_IsWrapped) {
        uint32_t refs = 0;
        m_Ext.m_AGS.DestroyDeviceD3D12(m_Ext.m_AGSContext, m_Device, &refs);

        // If released, suppress ComPtr
        if (!refs)
            m_Device.Nullify();
    }
}

template <typename Implementation, typename Interface, typename... Args>
Result DeviceD3D12::CreateImplementation(Interface*& entity, const Args&... args) {
    Implementation* implementation = Allocate<Implementation>(GetStdAllocator(), *this);
    Result result = implementation->Create(args...);

    if (result == Result::SUCCESS) {
        entity = (Interface*)implementation;
        return Result::SUCCESS;
    }

    Deallocate(GetStdAllocator(), implementation);

    return result;
}

Result DeviceD3D12::Create(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationD3D12Desc& deviceCreationD3D12Desc) {
    // IMPORTANT: Must be called before the D3D12 device is created, or the D3D12 runtime removes the device
    if (deviceCreationDesc.enableAPIValidation) {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            debugController->EnableDebugLayer();
    }

    // Get adapter
    ComPtr<IDXGIFactory4> dxgiFactory;
    HRESULT hr = CreateDXGIFactory2(deviceCreationDesc.enableAPIValidation ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&dxgiFactory));
    RETURN_ON_BAD_HRESULT(this, hr, "CreateDXGIFactory2()");

    if (deviceCreationDesc.adapterDesc) {
        LUID luid = *(LUID*)&deviceCreationDesc.adapterDesc->luid;
        hr = dxgiFactory->EnumAdapterByLuid(luid, IID_PPV_ARGS(&m_Adapter));
        RETURN_ON_BAD_HRESULT(this, hr, "IDXGIFactory4::EnumAdapterByLuid()");
    } else {
        hr = dxgiFactory->EnumAdapters(0, &m_Adapter);
        RETURN_ON_BAD_HRESULT(this, hr, "IDXGIFactory4::EnumAdapters()");
    }

    // Get adapter description as early as possible for meaningful error reporting
    DXGI_ADAPTER_DESC desc = {};
    hr = m_Adapter->GetDesc(&desc);
    RETURN_ON_BAD_HRESULT(this, hr, "IDXGIAdapter::GetDesc()");

    wcstombs(m_Desc.adapterDesc.description, desc.Description, GetCountOf(m_Desc.adapterDesc.description) - 1);
    m_Desc.adapterDesc.luid = *(uint64_t*)&desc.AdapterLuid;
    m_Desc.adapterDesc.videoMemorySize = desc.DedicatedVideoMemory;
    m_Desc.adapterDesc.systemMemorySize = desc.DedicatedSystemMemory + desc.SharedSystemMemory;
    m_Desc.adapterDesc.deviceId = desc.DeviceId;
    m_Desc.adapterDesc.vendor = GetVendorFromID(desc.VendorId);

    // Extensions
    if (m_Desc.adapterDesc.vendor == Vendor::NVIDIA)
        m_Ext.InitializeNVExt(this, deviceCreationD3D12Desc.isNVAPILoaded, deviceCreationD3D12Desc.d3d12Device != nullptr);
    else if (m_Desc.adapterDesc.vendor == Vendor::AMD)
        m_Ext.InitializeAMDExt(this, deviceCreationD3D12Desc.agsContext, deviceCreationD3D12Desc.d3d12Device != nullptr);

    // Device
    AGSDX12ReturnedParams agsParams = {};
    ComPtr<ID3D12DeviceBest> deviceTemp = (ID3D12DeviceBest*)deviceCreationD3D12Desc.d3d12Device;
    if (!deviceTemp) {
        uint32_t shaderExtRegister = deviceCreationDesc.shaderExtRegister ? deviceCreationDesc.shaderExtRegister : 63;

        if (m_Ext.HasAGS()) {
            AGSDX12DeviceCreationParams deviceCreationParams = {};
            deviceCreationParams.pAdapter = m_Adapter;
            deviceCreationParams.iid = __uuidof(ID3D12DeviceBest);
            deviceCreationParams.FeatureLevel = D3D_FEATURE_LEVEL_12_0;

            AGSDX12ExtensionParams extensionsParams = {};
            extensionsParams.uavSlot = shaderExtRegister;

            AGSReturnCode result = m_Ext.m_AGS.CreateDeviceD3D12(m_Ext.m_AGSContext, &deviceCreationParams, &extensionsParams, &agsParams);
            RETURN_ON_FAILURE(this, result == AGS_SUCCESS, Result::FAILURE, "agsDriverExtensionsDX11_CreateDevice() failed: %d", (int32_t)result);

            deviceTemp = (ID3D12DeviceBest*)agsParams.pDevice;
        } else {
            hr = D3D12CreateDevice(m_Adapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), (void**)&deviceTemp);
            RETURN_ON_BAD_HRESULT(this, hr, "D3D12CreateDevice()");

            // Register device
            if (m_Ext.HasNVAPI())
                NvAPI_D3D12_SetNvShaderExtnSlotSpace(m_Device, shaderExtRegister, deviceCreationDesc.shaderExtSpace);
        }
    }
    else
        m_IsWrapped = true;

    m_Version = QueryLatestDevice(deviceTemp, m_Device);
    REPORT_INFO(this, "Using ID3D12Device%u...", m_Version);

    if (deviceCreationDesc.enableAPIValidation) {
        ComPtr<ID3D12InfoQueue> pInfoQueue;
        m_Device->QueryInterface(&pInfoQueue);

        if (pInfoQueue) {
#ifdef _DEBUG
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif

            // TODO: this code is currently needed to disable known false-positive errors reported by the debug layer
            D3D12_MESSAGE_ID disableMessageIDs[] = {
                // It's almost impossible to match. Doesn't hurt perf on modern HW
                D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
                // Descriptor validation doesn't understand acceleration structures used outside of RAYGEN shaders
                D3D12_MESSAGE_ID_COMMAND_LIST_STATIC_DESCRIPTOR_RESOURCE_DIMENSION_MISMATCH,
            };

            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.pIDList = disableMessageIDs;
            filter.DenyList.NumIDs = GetCountOf(disableMessageIDs);
            pInfoQueue->AddStorageFilterEntries(&filter);
        }
    }

    // Wrap user-provided command queues
    if (deviceCreationD3D12Desc.d3d12GraphicsQueue)
        CreateCommandQueue(deviceCreationD3D12Desc.d3d12GraphicsQueue, m_CommandQueues[(uint32_t)CommandQueueType::GRAPHICS]);
    if (deviceCreationD3D12Desc.d3d12ComputeQueue)
        CreateCommandQueue(deviceCreationD3D12Desc.d3d12ComputeQueue, m_CommandQueues[(uint32_t)CommandQueueType::COMPUTE]);
    if (deviceCreationD3D12Desc.d3d12CopyQueue)
        CreateCommandQueue(deviceCreationD3D12Desc.d3d12CopyQueue, m_CommandQueues[(uint32_t)CommandQueueType::COPY]);

    // Check GRAPHICS queue availability
    CommandQueue* commandQueue;
    Result result = GetCommandQueue(CommandQueueType::GRAPHICS, commandQueue);
    if (result != Result::SUCCESS)
        return result;

    // Fill desc
    FillDesc(deviceCreationDesc.enableD3D12DrawParametersEmulation, agsParams);

    // Create indirect command signatures
    m_DispatchCommandSignature = CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH, sizeof(DispatchDesc), nullptr);
    if (m_Desc.isDispatchRaysIndirectSupported)
        m_DispatchRaysCommandSignature = CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS, sizeof(DispatchRaysIndirectDesc), nullptr);

    return FillFunctionTable(m_CoreInterface);
}

void DeviceD3D12::FillDesc(bool enableDrawParametersEmulation, const AGSDX12ReturnedParams& agsParams) {
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {D3D_HIGHEST_SHADER_MODEL};
    HRESULT hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(shaderModel) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS1 options1 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &options1, sizeof(options1));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options1) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS2 options2 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &options2, sizeof(options2));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options2) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS3 options3 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &options3, sizeof(options3));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options3) failed, result = 0x%08X!", hr);
    m_Desc.isCopyQueueTimestampSupported = options3.CopyQueueTimestampQueriesSupported;

    D3D12_FEATURE_DATA_D3D12_OPTIONS4 options4 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &options4, sizeof(options4));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options4) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options5) failed, result = 0x%08X!", hr);
    m_Desc.isRayTracingSupported = options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0;
    m_Desc.isDispatchRaysIndirectSupported = options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1;

    D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options6) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options7) failed, result = 0x%08X!", hr);
    m_Desc.isMeshShaderSupported = options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1;
    m_Desc.isDrawMeshTasksIndirectSupported = options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1;

#ifdef NRI_USE_AGILITY_SDK
    // Minimum supported client: Windows 10 Build 20348 (or Agility SDK)
    D3D12_FEATURE_DATA_D3D12_OPTIONS8 options8 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS8, &options8, sizeof(options8));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options8) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS9 options9 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS9, &options9, sizeof(options9));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options9) failed, result = 0x%08X!", hr);
    m_Desc.isMeshShaderPipelineStatsSupported = options9.MeshShaderPipelineStatsSupported;

    // Minimum supported client: Windows 11 Build 22000 (or Agility SDK)
    D3D12_FEATURE_DATA_D3D12_OPTIONS10 options10 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS10, &options10, sizeof(options10));
    if (FAILED(hr))
        REPORT_ERROR(this, "ID3D12Device::CheckFeatureSupport(options10) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS11 options11 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS11, &options11, sizeof(options11));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options11) failed, result = 0x%08X!", hr);

    // Minimum supported client: Windows 11 22H2 (or Agility SDK)
    D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options12) failed, result = 0x%08X!", hr);
    m_AreEnhancedBarriersSupported = options12.EnhancedBarriersSupported;

    D3D12_FEATURE_DATA_D3D12_OPTIONS13 options13 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS13, &options13, sizeof(options13));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options13) failed, result = 0x%08X!", hr);

    // Minimum supported client: Agility SDK
    D3D12_FEATURE_DATA_D3D12_OPTIONS14 options14 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS14, &options14, sizeof(options14));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options14) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS15 options15 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS15, &options15, sizeof(options15));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options15) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options16, sizeof(options16));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options16) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS17 options17 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS17, &options17, sizeof(options17));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options17) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS18 options18 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS18, &options18, sizeof(options18));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options18) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS19 options19 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS19, &options19, sizeof(options19));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options19) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS20 options20 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS20, &options20, sizeof(options20));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options20) failed, result = 0x%08X!", hr);

    D3D12_FEATURE_DATA_D3D12_OPTIONS21 options21 = {};
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS21, &options21, sizeof(options21));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(options21) failed, result = 0x%08X!", hr);
#endif

    const std::array<D3D_FEATURE_LEVEL, 5> levelsList = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_2,
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS levels = {};
    levels.NumFeatureLevels = (uint32_t)levelsList.size();
    levels.pFeatureLevelsRequested = levelsList.data();
    hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &levels, sizeof(levels));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D12Device::CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS) failed, result = 0x%08X!", hr);

    uint64_t timestampFrequency = 0;
    {
        CommandQueue* commandQueue = nullptr;
        Result result = GetCommandQueue(CommandQueueType::GRAPHICS, commandQueue);
        if (result == Result::SUCCESS) {
            ID3D12CommandQueue* commandQueueD3D12 = *(CommandQueueD3D12*)commandQueue;
            commandQueueD3D12->GetTimestampFrequency(&timestampFrequency);
        }
    }

    m_Desc.viewportMaxNum = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    m_Desc.viewportSubPixelBits = D3D12_SUBPIXEL_FRACTIONAL_BIT_COUNT;
    m_Desc.viewportBoundsRange[0] = D3D12_VIEWPORT_BOUNDS_MIN;
    m_Desc.viewportBoundsRange[1] = D3D12_VIEWPORT_BOUNDS_MAX;

    m_Desc.attachmentMaxDim = D3D12_REQ_RENDER_TO_BUFFER_WINDOW_WIDTH;
    m_Desc.attachmentLayerMaxNum = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    m_Desc.colorAttachmentMaxNum = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;

    m_Desc.colorSampleMaxNum = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.depthSampleMaxNum = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.stencilSampleMaxNum = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.zeroAttachmentsSampleMaxNum = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.textureColorSampleMaxNum = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.textureIntegerSampleMaxNum = 1;
    m_Desc.textureDepthSampleMaxNum = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.textureStencilSampleMaxNum = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.storageTextureSampleMaxNum = 1;

    m_Desc.texture1DMaxDim = D3D12_REQ_TEXTURE1D_U_DIMENSION;
    m_Desc.texture2DMaxDim = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    m_Desc.texture3DMaxDim = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    m_Desc.textureArrayMaxDim = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    m_Desc.texelBufferMaxDim = (1 << D3D12_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP) - 1;

#ifdef NRI_USE_AGILITY_SDK
    m_Desc.deviceUploadHeapSize = options16.GPUUploadHeapSupported ? m_Desc.adapterDesc.videoMemorySize : 0;
    m_Desc.uploadBufferTextureRowAlignment = options13.UnrestrictedBufferTextureCopyPitchSupported ? 1 : D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
    m_Desc.uploadBufferTextureSliceAlignment = options13.UnrestrictedBufferTextureCopyPitchSupported ? 1 : D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
#else
    m_Desc.uploadBufferTextureRowAlignment = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
    m_Desc.uploadBufferTextureSliceAlignment = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
#endif

    m_Desc.memoryAllocationMaxNum = 0xFFFFFFFF;
    m_Desc.samplerAllocationMaxNum = D3D12_REQ_SAMPLER_OBJECT_COUNT_PER_DEVICE;
    m_Desc.typedBufferOffsetAlignment = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;
    m_Desc.constantBufferOffsetAlignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    m_Desc.constantBufferMaxRange = D3D12_REQ_IMMEDIATE_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
    m_Desc.storageBufferOffsetAlignment = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;
    m_Desc.storageBufferMaxRange = (1 << D3D12_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP) - 1;
    m_Desc.bufferTextureGranularity = D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT; // TODO: 1024 in VK
    m_Desc.bufferMaxSize = D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM * 1024ull * 1024ull;
    m_Desc.pushConstantsMaxSize = D3D12_REQ_IMMEDIATE_CONSTANT_BUFFER_ELEMENT_COUNT * 16;

    m_Desc.boundDescriptorSetMaxNum = D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
    m_Desc.perStageDescriptorSamplerMaxNum = D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT;
    m_Desc.perStageDescriptorConstantBufferMaxNum = D3D12_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
    m_Desc.perStageDescriptorStorageBufferMaxNum = levels.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_11_1 ? D3D12_UAV_SLOT_COUNT : D3D12_PS_CS_UAV_REGISTER_COUNT;
    m_Desc.perStageDescriptorTextureMaxNum = D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
    m_Desc.perStageDescriptorStorageTextureMaxNum = levels.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_11_1 ? D3D12_UAV_SLOT_COUNT : D3D12_PS_CS_UAV_REGISTER_COUNT;
    m_Desc.perStageResourceMaxNum = D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;

    m_Desc.descriptorSetSamplerMaxNum = m_Desc.perStageDescriptorSamplerMaxNum;
    m_Desc.descriptorSetConstantBufferMaxNum = m_Desc.perStageDescriptorConstantBufferMaxNum;
    m_Desc.descriptorSetStorageBufferMaxNum = m_Desc.perStageDescriptorStorageBufferMaxNum;
    m_Desc.descriptorSetTextureMaxNum = m_Desc.perStageDescriptorTextureMaxNum;
    m_Desc.descriptorSetStorageTextureMaxNum = m_Desc.perStageDescriptorStorageTextureMaxNum;

    m_Desc.vertexShaderAttributeMaxNum = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    m_Desc.vertexShaderStreamMaxNum = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    m_Desc.vertexShaderOutputComponentMaxNum = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT * 4;

    m_Desc.tessControlShaderGenerationMaxLevel = D3D12_HS_MAXTESSFACTOR_UPPER_BOUND;
    m_Desc.tessControlShaderPatchPointMaxNum = D3D12_IA_PATCH_MAX_CONTROL_POINT_COUNT;
    m_Desc.tessControlShaderPerVertexInputComponentMaxNum = D3D12_HS_CONTROL_POINT_PHASE_INPUT_REGISTER_COUNT * D3D12_HS_CONTROL_POINT_REGISTER_COMPONENTS;
    m_Desc.tessControlShaderPerVertexOutputComponentMaxNum = D3D12_HS_CONTROL_POINT_PHASE_OUTPUT_REGISTER_COUNT * D3D12_HS_CONTROL_POINT_REGISTER_COMPONENTS;
    m_Desc.tessControlShaderPerPatchOutputComponentMaxNum = D3D12_HS_OUTPUT_PATCH_CONSTANT_REGISTER_SCALAR_COMPONENTS;
    m_Desc.tessControlShaderTotalOutputComponentMaxNum =
        m_Desc.tessControlShaderPatchPointMaxNum * m_Desc.tessControlShaderPerVertexOutputComponentMaxNum + m_Desc.tessControlShaderPerPatchOutputComponentMaxNum;

    m_Desc.tessEvaluationShaderInputComponentMaxNum = D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COUNT * D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COMPONENTS;
    m_Desc.tessEvaluationShaderOutputComponentMaxNum = D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COUNT * D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COMPONENTS;

    m_Desc.geometryShaderInvocationMaxNum = D3D12_GS_MAX_INSTANCE_COUNT;
    m_Desc.geometryShaderInputComponentMaxNum = D3D12_GS_INPUT_REGISTER_COUNT * D3D12_GS_INPUT_REGISTER_COMPONENTS;
    m_Desc.geometryShaderOutputComponentMaxNum = D3D12_GS_OUTPUT_REGISTER_COUNT * D3D12_GS_INPUT_REGISTER_COMPONENTS;
    m_Desc.geometryShaderOutputVertexMaxNum = D3D12_GS_MAX_OUTPUT_VERTEX_COUNT_ACROSS_INSTANCES;
    m_Desc.geometryShaderTotalOutputComponentMaxNum = D3D12_REQ_GS_INVOCATION_32BIT_OUTPUT_COMPONENT_LIMIT;

    m_Desc.fragmentShaderInputComponentMaxNum = D3D12_PS_INPUT_REGISTER_COUNT * D3D12_PS_INPUT_REGISTER_COMPONENTS;
    m_Desc.fragmentShaderOutputAttachmentMaxNum = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
    m_Desc.fragmentShaderDualSourceAttachmentMaxNum = 1;
    m_Desc.fragmentShaderCombinedOutputResourceMaxNum = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT + D3D12_PS_CS_UAV_REGISTER_COUNT;

    m_Desc.computeShaderSharedMemoryMaxSize = D3D12_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
    m_Desc.computeShaderWorkGroupMaxNum[0] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_Desc.computeShaderWorkGroupMaxNum[1] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_Desc.computeShaderWorkGroupMaxNum[2] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_Desc.computeShaderWorkGroupInvocationMaxNum = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
    m_Desc.computeShaderWorkGroupMaxDim[0] = D3D12_CS_THREAD_GROUP_MAX_X;
    m_Desc.computeShaderWorkGroupMaxDim[1] = D3D12_CS_THREAD_GROUP_MAX_Y;
    m_Desc.computeShaderWorkGroupMaxDim[2] = D3D12_CS_THREAD_GROUP_MAX_Z;

    if (m_Desc.isRayTracingSupported) {
        m_Desc.rayTracingShaderGroupIdentifierSize = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
        m_Desc.rayTracingShaderTableAligment = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        m_Desc.rayTracingShaderTableMaxStride = std::numeric_limits<uint64_t>::max();
        m_Desc.rayTracingShaderRecursionMaxDepth = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;
        m_Desc.rayTracingGeometryObjectMaxNum = (1 << 24) - 1;
    }

    if (m_Desc.isMeshShaderSupported) {
        m_Desc.meshControlSharedMemoryMaxSize = 32 * 1024;
        m_Desc.meshControlWorkGroupInvocationMaxNum = 128;
        m_Desc.meshControlPayloadMaxSize = 16 * 1024;
        m_Desc.meshEvaluationOutputVerticesMaxNum = 256;
        m_Desc.meshEvaluationOutputPrimitiveMaxNum = 256;
        m_Desc.meshEvaluationOutputComponentMaxNum = 128;
        m_Desc.meshEvaluationSharedMemoryMaxSize = 28 * 1024;
        m_Desc.meshEvaluationWorkGroupInvocationMaxNum = 128;
    }

    m_Desc.timestampFrequencyHz = timestampFrequency;
    m_Desc.subPixelPrecisionBits = D3D12_SUBPIXEL_FRACTIONAL_BIT_COUNT;
    m_Desc.subTexelPrecisionBits = D3D12_SUBTEXEL_FRACTIONAL_BIT_COUNT;
    m_Desc.mipmapPrecisionBits = D3D12_MIP_LOD_FRACTIONAL_BIT_COUNT;
    m_Desc.drawIndexedIndex16ValueMax = D3D12_16BIT_INDEX_STRIP_CUT_VALUE;
    m_Desc.drawIndexedIndex32ValueMax = D3D12_32BIT_INDEX_STRIP_CUT_VALUE;
    m_Desc.drawIndirectMaxNum = (1ull << D3D12_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP) - 1;
    m_Desc.samplerLodBiasMin = D3D12_MIP_LOD_BIAS_MIN;
    m_Desc.samplerLodBiasMax = D3D12_MIP_LOD_BIAS_MAX;
    m_Desc.samplerAnisotropyMax = D3D12_DEFAULT_MAX_ANISOTROPY;
    m_Desc.texelOffsetMin = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE;
    m_Desc.texelOffsetMax = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE;
    m_Desc.texelGatherOffsetMin = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE;
    m_Desc.texelGatherOffsetMax = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE;
    m_Desc.clipDistanceMaxNum = D3D12_CLIP_OR_CULL_DISTANCE_COUNT;
    m_Desc.cullDistanceMaxNum = D3D12_CLIP_OR_CULL_DISTANCE_COUNT;
    m_Desc.combinedClipAndCullDistanceMaxNum = D3D12_CLIP_OR_CULL_DISTANCE_COUNT;
    m_Desc.conservativeRasterTier = (uint8_t)options.ConservativeRasterizationTier;
    m_Desc.programmableSampleLocationsTier = (uint8_t)options2.ProgrammableSamplePositionsTier;

    m_Desc.isTextureFilterMinMaxSupported = levels.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_11_1 ? true : false;
    m_Desc.isLogicOpSupported = options.OutputMergerLogicOp != 0;
    m_Desc.isDepthBoundsTestSupported = options2.DepthBoundsTestSupported != 0;
    m_Desc.isComputeQueueSupported = true;
    m_Desc.isCopyQueueSupported = true;
    m_Desc.isDrawIndirectCountSupported = true;
#ifdef NRI_USE_AGILITY_SDK
    m_Desc.isIndependentFrontAndBackStencilReferenceAndMasksSupported = options14.IndependentFrontAndBackStencilRefMaskSupported ? true : false;
#endif
    m_Desc.isLineSmoothingSupported = true;
    m_Desc.isDrawParametersEmulationEnabled = enableDrawParametersEmulation && shaderModel.HighestShaderModel <= D3D_SHADER_MODEL_6_7;

    m_Desc.isShaderNativeI16Supported = options4.Native16BitShaderOpsSupported;
    m_Desc.isShaderNativeF16Supported = options4.Native16BitShaderOpsSupported;
    m_Desc.isShaderNativeI32Supported = true;
    m_Desc.isShaderNativeF32Supported = true;
    m_Desc.isShaderNativeI64Supported = options1.Int64ShaderOps;
    m_Desc.isShaderNativeF64Supported = options.DoublePrecisionFloatShaderOps;

    bool isShaderAtomicsF16Supported = false;
    NvAPI_D3D12_IsNvShaderExtnOpCodeSupported(m_Device, NV_EXTN_OP_FP16_ATOMIC, &isShaderAtomicsF16Supported);

    bool isShaderAtomicsF32Supported = false;
    NvAPI_D3D12_IsNvShaderExtnOpCodeSupported(m_Device, NV_EXTN_OP_FP32_ATOMIC, &isShaderAtomicsF32Supported);

    bool isShaderAtomicsI64Supported = false;
    NvAPI_D3D12_IsNvShaderExtnOpCodeSupported(m_Device, NV_EXTN_OP_UINT64_ATOMIC, &isShaderAtomicsI64Supported);

    m_Desc.isShaderAtomicsF16Supported = isShaderAtomicsF16Supported;
    m_Desc.isShaderAtomicsI32Supported = true;
    m_Desc.isShaderAtomicsF32Supported = isShaderAtomicsF32Supported;
#ifdef NRI_USE_AGILITY_SDK
    m_Desc.isShaderAtomicsI64Supported = isShaderAtomicsI64Supported || agsParams.extensionsSupported.intrinsics19 || options9.AtomicInt64OnTypedResourceSupported ||
                                         options9.AtomicInt64OnGroupSharedSupported || options11.AtomicInt64OnDescriptorHeapResourceSupported;
#else
    m_Desc.isShaderAtomicsI64Supported = isShaderAtomicsI64Supported || agsParams.extensionsSupported.intrinsics19;
#endif

    m_Desc.isSwapChainSupported = HasOutput();
    m_Desc.isLowLatencySupported = m_Ext.HasNVAPI();
}

Result DeviceD3D12::CreateCpuOnlyVisibleDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) {
    // IMPORTANT: m_FreeDescriptorLocks[type] must be acquired before calling this function
    ExclusiveScope lock(m_DescriptorHeapLock);

    size_t heapIndex = m_DescriptorHeaps.size();
    if (heapIndex >= HeapIndexType(-1))
        return Result::OUT_OF_MEMORY;

    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    D3D12_DESCRIPTOR_HEAP_DESC desc = {type, DESCRIPTORS_BATCH_SIZE, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, NRI_NODE_MASK};
    HRESULT hr = m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
    RETURN_ON_BAD_HRESULT(this, hr, "ID3D12Device::CreateDescriptorHeap()");

    DescriptorHeapDesc descriptorHeapDesc = {};
    descriptorHeapDesc.descriptorHeap = descriptorHeap;
    descriptorHeapDesc.descriptorPointerCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
    descriptorHeapDesc.descriptorSize = m_Device->GetDescriptorHandleIncrementSize(type);
    m_DescriptorHeaps.push_back(descriptorHeapDesc);

    auto& freeDescriptors = m_FreeDescriptors[type];
    for (uint32_t i = 0; i < desc.NumDescriptors; i++)
        freeDescriptors.push_back({(HeapIndexType)heapIndex, (HeapOffsetType)i});

    return Result::SUCCESS;
}

Result DeviceD3D12::GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, DescriptorHandle& descriptorHandle) {
    ExclusiveScope lock(m_FreeDescriptorLocks[type]);

    auto& freeDescriptors = m_FreeDescriptors[type];
    if (freeDescriptors.empty()) {
        Result result = CreateCpuOnlyVisibleDescriptorHeap(type);
        if (result != Result::SUCCESS)
            return result;
    }

    descriptorHandle = freeDescriptors.back();
    freeDescriptors.pop_back();

    return Result::SUCCESS;
}

DescriptorPointerCPU DeviceD3D12::GetDescriptorPointerCPU(const DescriptorHandle& descriptorHandle) {
    ExclusiveScope lock(m_DescriptorHeapLock);

    const DescriptorHeapDesc& descriptorHeapDesc = m_DescriptorHeaps[descriptorHandle.heapIndex];
    DescriptorPointerCPU descriptorPointer = descriptorHeapDesc.descriptorPointerCPU + descriptorHandle.heapOffset * descriptorHeapDesc.descriptorSize;

    return descriptorPointer;
}

void DeviceD3D12::GetMemoryInfo(MemoryLocation memoryLocation, const D3D12_RESOURCE_DESC& resourceDesc, MemoryDesc& memoryDesc) const {
    if (memoryLocation == MemoryLocation::DEVICE_UPLOAD && m_Desc.deviceUploadHeapSize == 0)
        memoryLocation = MemoryLocation::HOST_UPLOAD;

    memoryDesc.type = GetMemoryType(memoryLocation, resourceDesc);

    D3D12_RESOURCE_ALLOCATION_INFO resourceAllocationInfo = m_Device->GetResourceAllocationInfo(NRI_NODE_MASK, 1, &resourceDesc);
    memoryDesc.size = (uint64_t)resourceAllocationInfo.SizeInBytes;
    memoryDesc.alignment = (uint32_t)resourceAllocationInfo.Alignment;

    memoryDesc.mustBeDedicated = RequiresDedicatedAllocation(memoryDesc.type);
}

ComPtr<ID3D12CommandSignature> DeviceD3D12::CreateCommandSignature(
    D3D12_INDIRECT_ARGUMENT_TYPE indirectArgumentType, uint32_t stride, ID3D12RootSignature* rootSignature, bool enableDrawParametersEmulation) {
    const bool isDrawArgument =
        enableDrawParametersEmulation && (indirectArgumentType == D3D12_INDIRECT_ARGUMENT_TYPE_DRAW || indirectArgumentType == D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED);

    D3D12_INDIRECT_ARGUMENT_DESC indirectArgumentDescs[2] = {};
    if (isDrawArgument) {
        // Draw base parameters emulation
        // Base on: https://github.com/google/dawn/blob/e72fa969ad72e42064cd33bd99572ea12b0bcdaf/src/dawn/native/d3d12/PipelineLayoutD3D12.cpp#L504
        indirectArgumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        indirectArgumentDescs[0].Constant.RootParameterIndex = 0;
        indirectArgumentDescs[0].Constant.DestOffsetIn32BitValues = 0;
        indirectArgumentDescs[0].Constant.Num32BitValuesToSet = 2;

        indirectArgumentDescs[1].Type = indirectArgumentType;
    } else
        indirectArgumentDescs[0].Type = indirectArgumentType;

    D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
    commandSignatureDesc.NumArgumentDescs = isDrawArgument ? 2 : 1;
    commandSignatureDesc.pArgumentDescs = indirectArgumentDescs;
    commandSignatureDesc.NodeMask = NRI_NODE_MASK;
    commandSignatureDesc.ByteStride = stride;

    ComPtr<ID3D12CommandSignature> commandSignature = nullptr;
    HRESULT hr = m_Device->CreateCommandSignature(&commandSignatureDesc, isDrawArgument ? rootSignature : nullptr, IID_PPV_ARGS(&commandSignature));
    if (FAILED(hr))
        REPORT_ERROR(this, "ID3D12Device::CreateCommandSignature() failed, result = 0x%08X!", hr);

    return commandSignature;
}

ID3D12CommandSignature* DeviceD3D12::GetDrawCommandSignature(uint32_t stride, ID3D12RootSignature* rootSignature) {
    auto key = HashRootSignatureAndStride(rootSignature, stride);
    auto commandSignatureIt = m_DrawCommandSignatures.find(key);
    if (commandSignatureIt != m_DrawCommandSignatures.end())
        return commandSignatureIt->second;

    ComPtr<ID3D12CommandSignature> commandSignature = CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW, stride, rootSignature);
    m_DrawCommandSignatures[key] = commandSignature;

    return commandSignature;
}

ID3D12CommandSignature* DeviceD3D12::GetDrawIndexedCommandSignature(uint32_t stride, ID3D12RootSignature* rootSignature) {
    auto key = HashRootSignatureAndStride(rootSignature, stride);
    auto commandSignatureIt = m_DrawIndexedCommandSignatures.find(key);
    if (commandSignatureIt != m_DrawIndexedCommandSignatures.end())
        return commandSignatureIt->second;

    ComPtr<ID3D12CommandSignature> commandSignature = CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, stride, rootSignature);
    m_DrawIndexedCommandSignatures[key] = commandSignature;

    return commandSignature;
}

ID3D12CommandSignature* DeviceD3D12::GetDrawMeshCommandSignature(uint32_t stride) {
    auto commandSignatureIt = m_DrawMeshCommandSignatures.find(stride);
    if (commandSignatureIt != m_DrawMeshCommandSignatures.end())
        return commandSignatureIt->second;

    ComPtr<ID3D12CommandSignature> commandSignature = CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH, stride, nullptr);
    m_DrawMeshCommandSignatures[stride] = commandSignature;

    return commandSignature;
}

ID3D12CommandSignature* DeviceD3D12::GetDispatchRaysCommandSignature() const {
    return m_DispatchRaysCommandSignature.GetInterface();
}

ID3D12CommandSignature* DeviceD3D12::GetDispatchCommandSignature() const {
    return m_DispatchCommandSignature.GetInterface();
}

MemoryType DeviceD3D12::GetMemoryType(MemoryLocation memoryLocation, const D3D12_RESOURCE_DESC& resourceDesc) const {
    return ::GetMemoryType(memoryLocation, resourceDesc);
}

//================================================================================================================
// DeviceBase
//================================================================================================================

void DeviceD3D12::Destroy() {
    Deallocate(GetStdAllocator(), this);
}

//================================================================================================================
// NRI
//================================================================================================================

inline Result DeviceD3D12::CreateSwapChain(const SwapChainDesc& swapChainDesc, SwapChain*& swapChain) {
    return CreateImplementation<SwapChainD3D12>(swapChain, swapChainDesc);
}

inline void DeviceD3D12::DestroySwapChain(SwapChain& swapChain) {
    Deallocate(GetStdAllocator(), (SwapChainD3D12*)&swapChain);
}

inline Result DeviceD3D12::GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue) {
    ExclusiveScope lock(m_QueueLock);

    uint32_t queueIndex = (uint32_t)commandQueueType;
    if (m_CommandQueues[queueIndex]) {
        commandQueue = (CommandQueue*)m_CommandQueues[queueIndex];
        return Result::SUCCESS;
    }

    Result result = CreateCommandQueue(commandQueueType, commandQueue);
    if (result == Result::SUCCESS)
        m_CommandQueues[queueIndex] = (CommandQueueD3D12*)commandQueue;
    else
        REPORT_WARNING(this, "%s command queue is not supported by the device!",
            commandQueueType == CommandQueueType::GRAPHICS ? "GRAPHICS" : (commandQueueType == CommandQueueType::COMPUTE ? "COMPUTE" : "COPY"));

    return result;
}

inline Result DeviceD3D12::CreateCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue) {
    return CreateImplementation<CommandQueueD3D12>(commandQueue, commandQueueType);
}

inline Result DeviceD3D12::CreateCommandQueue(void* d3d12commandQueue, CommandQueueD3D12*& commandQueue) {
    return CreateImplementation<CommandQueueD3D12>(commandQueue, (ID3D12CommandQueue*)d3d12commandQueue);
}

inline Result DeviceD3D12::CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator) {
    return CreateImplementation<CommandAllocatorD3D12>(commandAllocator, commandQueue);
}

inline Result DeviceD3D12::CreateDescriptorPool(const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    return CreateImplementation<DescriptorPoolD3D12>(descriptorPool, descriptorPoolDesc);
}

inline Result DeviceD3D12::CreateBuffer(const BufferDesc& bufferDesc, Buffer*& buffer) {
    return CreateImplementation<BufferD3D12>(buffer, bufferDesc);
}

inline Result DeviceD3D12::CreateTexture(const TextureDesc& textureDesc, Texture*& texture) {
    return CreateImplementation<TextureD3D12>(texture, textureDesc);
}

inline Result DeviceD3D12::CreateDescriptor(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
    return CreateImplementation<DescriptorD3D12>(bufferView, bufferViewDesc);
}

inline Result DeviceD3D12::CreateDescriptor(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
    return CreateImplementation<DescriptorD3D12>(textureView, textureViewDesc);
}

inline Result DeviceD3D12::CreateDescriptor(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
    return CreateImplementation<DescriptorD3D12>(textureView, textureViewDesc);
}

inline Result DeviceD3D12::CreateDescriptor(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
    return CreateImplementation<DescriptorD3D12>(textureView, textureViewDesc);
}

Result DeviceD3D12::CreateDescriptor(const AccelerationStructure& accelerationStructure, Descriptor*& accelerationStructureView) { // TODO: not inline
    return CreateImplementation<DescriptorD3D12>(accelerationStructureView, accelerationStructure);
}

inline Result DeviceD3D12::CreateDescriptor(const SamplerDesc& samplerDesc, Descriptor*& sampler) {
    return CreateImplementation<DescriptorD3D12>(sampler, samplerDesc);
}

inline Result DeviceD3D12::CreatePipelineLayout(const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout) {
    return CreateImplementation<PipelineLayoutD3D12>(pipelineLayout, pipelineLayoutDesc);
}

inline Result DeviceD3D12::CreatePipeline(const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline) {
    return CreateImplementation<PipelineD3D12>(pipeline, graphicsPipelineDesc);
}

inline Result DeviceD3D12::CreatePipeline(const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline) {
    return CreateImplementation<PipelineD3D12>(pipeline, computePipelineDesc);
}

inline Result DeviceD3D12::CreatePipeline(const RayTracingPipelineDesc& rayTracingPipelineDesc, Pipeline*& pipeline) {
    return CreateImplementation<PipelineD3D12>(pipeline, rayTracingPipelineDesc);
}

inline Result DeviceD3D12::CreateFence(uint64_t initialValue, Fence*& fence) {
    return CreateImplementation<FenceD3D12>(fence, initialValue);
}

inline Result DeviceD3D12::CreateQueryPool(const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool) {
    return CreateImplementation<QueryPoolD3D12>(queryPool, queryPoolDesc);
}

inline Result DeviceD3D12::CreateCommandBuffer(const CommandBufferD3D12Desc& commandBufferDesc, CommandBuffer*& commandBuffer) {
    return CreateImplementation<CommandBufferD3D12>(commandBuffer, commandBufferDesc);
}

Result DeviceD3D12::CreateBuffer(const BufferD3D12Desc& bufferDesc, Buffer*& buffer) { // TODO: not inline
    return CreateImplementation<BufferD3D12>(buffer, bufferDesc);
}

inline Result DeviceD3D12::CreateTexture(const TextureD3D12Desc& textureDesc, Texture*& texture) {
    return CreateImplementation<TextureD3D12>(texture, textureDesc);
}

inline Result DeviceD3D12::CreateMemory(const MemoryD3D12Desc& memoryDesc, Memory*& memory) {
    return CreateImplementation<MemoryD3D12>(memory, memoryDesc);
}

inline void DeviceD3D12::DestroyCommandAllocator(CommandAllocator& commandAllocator) {
    Deallocate(GetStdAllocator(), (CommandAllocatorD3D12*)&commandAllocator);
}

inline void DeviceD3D12::DestroyDescriptorPool(DescriptorPool& descriptorPool) {
    Deallocate(GetStdAllocator(), (DescriptorPoolD3D12*)&descriptorPool);
}

inline void DeviceD3D12::DestroyBuffer(Buffer& buffer) {
    Deallocate(GetStdAllocator(), (BufferD3D12*)&buffer);
}

inline void DeviceD3D12::DestroyTexture(Texture& texture) {
    Deallocate(GetStdAllocator(), (TextureD3D12*)&texture);
}

inline void DeviceD3D12::DestroyDescriptor(Descriptor& descriptor) {
    Deallocate(GetStdAllocator(), (DescriptorD3D12*)&descriptor);
}

inline void DeviceD3D12::DestroyPipelineLayout(PipelineLayout& pipelineLayout) {
    Deallocate(GetStdAllocator(), (PipelineLayoutD3D12*)&pipelineLayout);
}

inline void DeviceD3D12::DestroyPipeline(Pipeline& pipeline) {
    Deallocate(GetStdAllocator(), (PipelineD3D12*)&pipeline);
}

inline void DeviceD3D12::DestroyFence(Fence& fence) {
    Deallocate(GetStdAllocator(), (FenceD3D12*)&fence);
}

inline void DeviceD3D12::DestroyQueryPool(QueryPool& queryPool) {
    Deallocate(GetStdAllocator(), (QueryPoolD3D12*)&queryPool);
}

inline Result DeviceD3D12::AllocateMemory(const MemoryType memoryType, uint64_t size, Memory*& memory) {
    return CreateImplementation<MemoryD3D12>(memory, memoryType, size);
}

inline Result DeviceD3D12::BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        Result result = ((BufferD3D12*)memoryBindingDescs[i].buffer)->BindMemory((MemoryD3D12*)memoryBindingDescs[i].memory, memoryBindingDescs[i].offset);
        if (result != Result::SUCCESS)
            return result;
    }

    return Result::SUCCESS;
}

inline Result DeviceD3D12::BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        Result result = ((TextureD3D12*)memoryBindingDescs[i].texture)->BindMemory((MemoryD3D12*)memoryBindingDescs[i].memory, memoryBindingDescs[i].offset);
        if (result != Result::SUCCESS)
            return result;
    }

    return Result::SUCCESS;
}

inline Result DeviceD3D12::BindAccelerationStructureMemory(const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        Result result = ((AccelerationStructureD3D12*)memoryBindingDescs[i].accelerationStructure)->BindMemory(memoryBindingDescs[i].memory, memoryBindingDescs[i].offset);
        if (result != Result::SUCCESS)
            return result;
    }

    return Result::SUCCESS;
}

inline void DeviceD3D12::FreeMemory(Memory& memory) {
    Deallocate(GetStdAllocator(), (MemoryD3D12*)&memory);
}

inline FormatSupportBits DeviceD3D12::GetFormatSupport(Format format) const {
    FormatSupportBits mask = FormatSupportBits::UNSUPPORTED;

    D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = {GetDxgiFormat(format).typed};
    HRESULT hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport));

    if (SUCCEEDED(hr)) {
#define UPDATE_SUPPORT_BITS(required, optional, bit) \
    if ((formatSupport.Support1 & (required)) == (required) && ((formatSupport.Support1 & (optional)) != 0 || (optional) == 0)) \
        mask |= bit;

        UPDATE_SUPPORT_BITS(0, D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE | D3D12_FORMAT_SUPPORT1_SHADER_LOAD, FormatSupportBits::TEXTURE);
        UPDATE_SUPPORT_BITS(D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW, 0, FormatSupportBits::STORAGE_TEXTURE);
        UPDATE_SUPPORT_BITS(D3D12_FORMAT_SUPPORT1_RENDER_TARGET, 0, FormatSupportBits::COLOR_ATTACHMENT);
        UPDATE_SUPPORT_BITS(D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL, 0, FormatSupportBits::DEPTH_STENCIL_ATTACHMENT);
        UPDATE_SUPPORT_BITS(D3D12_FORMAT_SUPPORT1_BLENDABLE, 0, FormatSupportBits::BLEND);

        UPDATE_SUPPORT_BITS(D3D12_FORMAT_SUPPORT1_BUFFER, D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE | D3D12_FORMAT_SUPPORT1_SHADER_LOAD, FormatSupportBits::BUFFER);
        UPDATE_SUPPORT_BITS(D3D12_FORMAT_SUPPORT1_BUFFER | D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW, 0, FormatSupportBits::STORAGE_BUFFER);
        UPDATE_SUPPORT_BITS(D3D12_FORMAT_SUPPORT1_IA_VERTEX_BUFFER, 0, FormatSupportBits::VERTEX_BUFFER);

#undef UPDATE_SUPPORT_BITS

#define UPDATE_SUPPORT_BITS(optional, bit) \
    if ((formatSupport.Support2 & (optional)) != 0) \
        mask |= bit;

        const uint32_t anyAtomics = D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_ADD | D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_BITWISE_OPS |
                                    D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_COMPARE_STORE_OR_COMPARE_EXCHANGE | D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_EXCHANGE |
                                    D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_SIGNED_MIN_OR_MAX | D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_UNSIGNED_MIN_OR_MAX;

        if (mask & FormatSupportBits::STORAGE_TEXTURE)
            UPDATE_SUPPORT_BITS(anyAtomics, FormatSupportBits::STORAGE_TEXTURE_ATOMICS);

        if (mask & FormatSupportBits::STORAGE_BUFFER)
            UPDATE_SUPPORT_BITS(anyAtomics, FormatSupportBits::STORAGE_BUFFER_ATOMICS);

#undef UPDATE_SUPPORT_BITS
    }

    return mask;
}

inline uint32_t DeviceD3D12::CalculateAllocationNumber(const ResourceGroupDesc& resourceGroupDesc) const {
    HelperDeviceMemoryAllocator allocator(m_CoreInterface, (Device&)*this);

    return allocator.CalculateAllocationNumber(resourceGroupDesc);
}

inline Result DeviceD3D12::AllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations) {
    HelperDeviceMemoryAllocator allocator(m_CoreInterface, (Device&)*this);

    return allocator.AllocateAndBindMemory(resourceGroupDesc, allocations);
}

inline Result DeviceD3D12::CreateAccelerationStructure(const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return CreateImplementation<AccelerationStructureD3D12>(accelerationStructure, accelerationStructureDesc);
}

inline Result DeviceD3D12::CreateAccelerationStructure(const AccelerationStructureD3D12Desc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return CreateImplementation<AccelerationStructureD3D12>(accelerationStructure, accelerationStructureDesc);
}

inline void DeviceD3D12::DestroyAccelerationStructure(AccelerationStructure& accelerationStructure) {
    Deallocate(GetStdAllocator(), (AccelerationStructureD3D12*)&accelerationStructure);
}

Result DeviceD3D12::CreateDefaultDrawSignatures(ID3D12RootSignature* rootSignature, bool enableDrawParametersEmulation) {
    bool drawParametersEmulation = m_Desc.isDrawParametersEmulationEnabled && enableDrawParametersEmulation;
    const uint32_t drawStride = drawParametersEmulation ? sizeof(nri::DrawBaseDesc) : sizeof(nri::DrawDesc);
    const uint32_t drawIndexedStride = drawParametersEmulation ? sizeof(nri::DrawIndexedBaseDesc) : sizeof(nri::DrawIndexedDesc);

    ComPtr<ID3D12CommandSignature> drawCommandSignature = CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW, drawStride, rootSignature, drawParametersEmulation);
    if (!drawCommandSignature)
        return nri::Result::FAILURE;

    auto key = HashRootSignatureAndStride(rootSignature, drawStride);
    m_DrawCommandSignatures.emplace(key, drawCommandSignature);

    ComPtr<ID3D12CommandSignature> drawIndexedCommandSignature =
        CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, drawIndexedStride, rootSignature, drawParametersEmulation);
    if (!drawIndexedCommandSignature)
        return nri::Result::FAILURE;

    key = HashRootSignatureAndStride(rootSignature, drawIndexedStride);
    m_DrawIndexedCommandSignatures.emplace(key, drawIndexedCommandSignature);

    return nri::Result::SUCCESS;
}

namespace d3d12 {
#include "D3DExt.hpp"
}

#include "DeviceD3D12.hpp"
