// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"

#include "BufferD3D11.h"
#include "CommandAllocatorD3D11.h"
#include "CommandQueueD3D11.h"
#include "DescriptorD3D11.h"
#include "DescriptorPoolD3D11.h"
#include "DeviceD3D11.h"
#include "FenceD3D11.h"
#include "HelperDeviceMemoryAllocator.h"
#include "MemoryD3D11.h"
#include "PipelineD3D11.h"
#include "PipelineLayoutD3D11.h"
#include "QueryPoolD3D11.h"
#include "Streamer.h"
#include "SwapChainD3D11.h"
#include "TextureD3D11.h"

using namespace nri;

uint8_t QueryLatestDeviceContext(ComPtr<ID3D11DeviceContextBest>& in, ComPtr<ID3D11DeviceContextBest>& out);

static uint8_t QueryLatestDevice(ComPtr<ID3D11DeviceBest>& in, ComPtr<ID3D11DeviceBest>& out) {
    static const IID versions[] = {
        __uuidof(ID3D11Device5),
        __uuidof(ID3D11Device4),
        __uuidof(ID3D11Device3),
        __uuidof(ID3D11Device2),
        __uuidof(ID3D11Device1),
        __uuidof(ID3D11Device),
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

Result CreateDeviceD3D11(const DeviceCreationDesc& deviceCreationDesc, DeviceBase*& device) {
    StdAllocator<uint8_t> allocator(deviceCreationDesc.memoryAllocatorInterface);

    DeviceD3D11* implementation = Allocate<DeviceD3D11>(allocator, deviceCreationDesc.callbackInterface, allocator);
    Result result = implementation->Create(deviceCreationDesc, nullptr, nullptr, false);

    if (result == Result::SUCCESS) {
        device = (DeviceBase*)implementation;
        return Result::SUCCESS;
    }

    Deallocate(allocator, implementation);

    return result;
}

Result CreateDeviceD3D11(const DeviceCreationD3D11Desc& deviceCreationD3D11Desc, DeviceBase*& device) {
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.callbackInterface = deviceCreationD3D11Desc.callbackInterface;
    deviceCreationDesc.memoryAllocatorInterface = deviceCreationD3D11Desc.memoryAllocatorInterface;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::D3D11;

    StdAllocator<uint8_t> allocator(deviceCreationDesc.memoryAllocatorInterface);

    DeviceD3D11* implementation = Allocate<DeviceD3D11>(allocator, deviceCreationDesc.callbackInterface, allocator);
    Result result = implementation->Create(deviceCreationDesc, deviceCreationD3D11Desc.d3d11Device, deviceCreationD3D11Desc.agsContext, deviceCreationD3D11Desc.isNVAPILoaded);

    if (result == Result::SUCCESS) {
        device = (DeviceBase*)implementation;
        return Result::SUCCESS;
    }

    Deallocate(allocator, implementation);

    return result;
}

DeviceD3D11::DeviceD3D11(const CallbackInterface& callbacks, StdAllocator<uint8_t>& stdAllocator) : DeviceBase(callbacks, stdAllocator), m_CommandQueues(GetStdAllocator()) {
    m_Desc.graphicsAPI = GraphicsAPI::D3D11;
    m_Desc.nriVersionMajor = NRI_VERSION_MAJOR;
    m_Desc.nriVersionMinor = NRI_VERSION_MINOR;
}

DeviceD3D11::~DeviceD3D11() {
    if (m_ImmediateContext)
        GetExt()->EndUAVOverlap(m_ImmediateContext);

    DeleteCriticalSection(&m_CriticalSection);
}

Result DeviceD3D11::Create(const DeviceCreationDesc& deviceCreationDesc, ID3D11Device* device, AGSContext* agsContext, bool isNVAPILoadedInApp) {
    // Get adapter
    if (!device) {
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
    } else {
        ComPtr<IDXGIDevice> dxgiDevice;
        HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
        RETURN_ON_BAD_HRESULT(this, hr, "QueryInterface(IDXGIDevice)");

        hr = dxgiDevice->GetAdapter(&m_Adapter);
        RETURN_ON_BAD_HRESULT(this, hr, "IDXGIDevice::GetAdapter()");
    }

    // Get adapter description as early as possible for meaningful error reporting
    DXGI_ADAPTER_DESC desc = {};
    HRESULT hr = m_Adapter->GetDesc(&desc);
    RETURN_ON_BAD_HRESULT(this, hr, "IDXGIAdapter::GetDesc()");

    wcstombs(m_Desc.adapterDesc.description, desc.Description, GetCountOf(m_Desc.adapterDesc.description) - 1);
    m_Desc.adapterDesc.luid = *(uint64_t*)&desc.AdapterLuid;
    m_Desc.adapterDesc.videoMemorySize = desc.DedicatedVideoMemory;
    m_Desc.adapterDesc.systemMemorySize = desc.DedicatedSystemMemory + desc.SharedSystemMemory;
    m_Desc.adapterDesc.deviceId = desc.DeviceId;
    m_Desc.adapterDesc.vendor = GetVendorFromID(desc.VendorId);

    // Extensions
    if (m_Desc.adapterDesc.vendor == Vendor::NVIDIA)
        m_Ext.InitializeNVExt(this, isNVAPILoadedInApp, device != nullptr);
    else if (m_Desc.adapterDesc.vendor == Vendor::AMD)
        m_Ext.InitializeAMDExt(this, agsContext, device != nullptr);

    // Device
    AGSDX11ReturnedParams params = {};
    if (m_Desc.adapterDesc.vendor == Vendor::NVIDIA) {
        // Tricky part: AGSDX11ReturnedParams struct is used to properly propagate depthBoundsTest support
        params.extensionsSupported.depthBoundsDeferredContexts = true;
        params.extensionsSupported.depthBoundsTest = true;
        params.extensionsSupported.uavOverlap = true;
        params.extensionsSupported.UAVOverlapDeferredContexts = true;
    }

    ComPtr<ID3D11DeviceBest> deviceTemp = (ID3D11DeviceBest*)device;
    if (!device) {
        const UINT flags = deviceCreationDesc.enableAPIValidation ? D3D11_CREATE_DEVICE_DEBUG : 0;
        const std::array<D3D_FEATURE_LEVEL, 2> levels = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};

        if (m_Ext.HasAGS()) {
            m_Ext.CreateDeviceUsingAGS(m_Adapter, levels.data(), levels.size(), flags, params);

            device = params.pDevice;
            if (!device)
                return Result::FAILURE;
        } else {
            hr = D3D11CreateDevice(
                m_Adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, levels.data(), (uint32_t)levels.size(), D3D11_SDK_VERSION, (ID3D11Device**)&deviceTemp, nullptr, nullptr);

            // If Debug Layer is not available, try without D3D11_CREATE_DEVICE_DEBUG
            if (flags && (uint32_t)hr == 0x887a002d)
                hr = D3D11CreateDevice(
                    m_Adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, levels.data(), (uint32_t)levels.size(), D3D11_SDK_VERSION, (ID3D11Device**)&deviceTemp, nullptr, nullptr);

            RETURN_ON_BAD_HRESULT(this, hr, "D3D11CreateDevice()");
        }
    }

    m_Version = QueryLatestDevice(deviceTemp, m_Device);
    REPORT_INFO(this, "Using ID3D11Device%u...", m_Version);

    // Immediate context
    ComPtr<ID3D11DeviceContextBest> immediateContext;
    m_Device->GetImmediateContext((ID3D11DeviceContext**)&immediateContext);

    m_ImmediateContextVersion = QueryLatestDeviceContext(immediateContext, m_ImmediateContext);
    REPORT_INFO(this, "Using ID3D11DeviceContext%u...", m_ImmediateContextVersion);

    // Set ShaderExt UAV slot
    if (m_Ext.HasNVAPI())
        NvAPI_D3D11_SetNvShaderExtnSlot(m_Device, SHADER_EXT_UAV_SLOT);

    // Skip UAV barriers by default on the immediate context
    GetExt()->BeginUAVOverlap(m_ImmediateContext);

    // Threading
    D3D11_FEATURE_DATA_THREADING threadingCaps = {};
    hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_THREADING, &threadingCaps, sizeof(threadingCaps));
    if (FAILED(hr) || !threadingCaps.DriverConcurrentCreates)
        REPORT_WARNING(this, "Concurrent resource creation is not supported by the driver!");

    m_IsDeferredContextEmulated = !m_Ext.HasNVAPI() || deviceCreationDesc.enableD3D11CommandBufferEmulation;
    if (!threadingCaps.DriverCommandLists) {
        REPORT_WARNING(this, "Deferred Contexts are not supported by the driver and will be emulated!");
        m_IsDeferredContextEmulated = true;

        params.extensionsSupported.UAVOverlapDeferredContexts = params.extensionsSupported.uavOverlap;
        params.extensionsSupported.depthBoundsDeferredContexts = params.extensionsSupported.depthBoundsTest;
    }

    hr = m_ImmediateContext->QueryInterface(IID_PPV_ARGS(&m_Multithread));
    if (FAILED(hr)) {
        REPORT_WARNING(this, "ID3D11Multithread is not supported: a critical section will be used instead!");
        InitializeCriticalSection(&m_CriticalSection);
    } else
        m_Multithread->SetMultithreadProtected(true);

    // Other
    FillDesc(params);

    for (uint32_t i = 0; i < (uint32_t)CommandQueueType::MAX_NUM; i++)
        m_CommandQueues.emplace_back(*this);

    return FillFunctionTable(m_CoreInterface);
}

void DeviceD3D11::FillDesc(const AGSDX11ReturnedParams& params) {
    D3D11_FEATURE_DATA_D3D11_OPTIONS options = {};
    HRESULT hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &options, sizeof(options));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D11Device::CheckFeatureSupport(options) failed, result = 0x%08X!", hr);

    D3D11_FEATURE_DATA_D3D11_OPTIONS1 options1 = {};
    hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS1, &options1, sizeof(options1));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D11Device::CheckFeatureSupport(options1) failed, result = 0x%08X!", hr);

    D3D11_FEATURE_DATA_D3D11_OPTIONS2 options2 = {};
    hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &options2, sizeof(options2));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D11Device::CheckFeatureSupport(options2) failed, result = 0x%08X!", hr);

    D3D11_FEATURE_DATA_D3D11_OPTIONS3 options3 = {};
    hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &options3, sizeof(options3));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D11Device::CheckFeatureSupport(options3) failed, result = 0x%08X!", hr);

    D3D11_FEATURE_DATA_D3D11_OPTIONS4 options4 = {};
    hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS4, &options4, sizeof(options4));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D11Device::CheckFeatureSupport(options4) failed, result = 0x%08X!", hr);

    D3D11_FEATURE_DATA_D3D11_OPTIONS5 options5 = {};
    hr = m_Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS5, &options5, sizeof(options5));
    if (FAILED(hr))
        REPORT_WARNING(this, "ID3D11Device::CheckFeatureSupport(options5) failed, result = 0x%08X!", hr);

    uint64_t timestampFrequency = 0;
    {
        D3D11_QUERY_DESC queryDesc = {};
        queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;

        ComPtr<ID3D11Query> query;
        hr = m_Device->CreateQuery(&queryDesc, &query);
        if (SUCCEEDED(hr)) {
            m_ImmediateContext->Begin(query);
            m_ImmediateContext->End(query);

            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data = {};
            while (m_ImmediateContext->GetData(query, &data, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0) == S_FALSE)
                ;

            timestampFrequency = data.Frequency;
        }
    }

    m_Desc.viewportMaxNum = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    m_Desc.viewportSubPixelBits = D3D11_SUBPIXEL_FRACTIONAL_BIT_COUNT;
    m_Desc.viewportBoundsRange[0] = D3D11_VIEWPORT_BOUNDS_MIN;
    m_Desc.viewportBoundsRange[1] = D3D11_VIEWPORT_BOUNDS_MAX;

    m_Desc.attachmentMaxDim = D3D11_REQ_RENDER_TO_BUFFER_WINDOW_WIDTH;
    m_Desc.attachmentLayerMaxNum = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    m_Desc.colorAttachmentMaxNum = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;

    m_Desc.colorSampleMaxNum = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.depthSampleMaxNum = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.stencilSampleMaxNum = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.zeroAttachmentsSampleMaxNum = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.textureColorSampleMaxNum = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.textureIntegerSampleMaxNum = 1;
    m_Desc.textureDepthSampleMaxNum = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.textureStencilSampleMaxNum = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;
    m_Desc.storageTextureSampleMaxNum = 1;

    m_Desc.texture1DMaxDim = D3D11_REQ_TEXTURE1D_U_DIMENSION;
    m_Desc.texture2DMaxDim = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    m_Desc.texture3DMaxDim = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    m_Desc.textureArrayMaxDim = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    m_Desc.texelBufferMaxDim = (1 << D3D11_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP) - 1;

    m_Desc.memoryAllocationMaxNum = 0xFFFFFFFF;
    m_Desc.samplerAllocationMaxNum = D3D11_REQ_SAMPLER_OBJECT_COUNT_PER_DEVICE;
    m_Desc.uploadBufferTextureRowAlignment = 256;   // D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
    m_Desc.uploadBufferTextureSliceAlignment = 512; // D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
    m_Desc.typedBufferOffsetAlignment = D3D11_RAW_UAV_SRV_BYTE_ALIGNMENT;
    m_Desc.constantBufferOffsetAlignment = 256; // D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    m_Desc.constantBufferMaxRange = D3D11_REQ_IMMEDIATE_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
    m_Desc.storageBufferOffsetAlignment = D3D11_RAW_UAV_SRV_BYTE_ALIGNMENT;
    m_Desc.storageBufferMaxRange = (1 << D3D11_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP) - 1;
    m_Desc.bufferTextureGranularity = 1;
    m_Desc.bufferMaxSize = D3D11_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM * 1024ull * 1024ull;
    m_Desc.pushConstantsMaxSize = D3D11_REQ_IMMEDIATE_CONSTANT_BUFFER_ELEMENT_COUNT * 16;

    m_Desc.boundDescriptorSetMaxNum = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
    m_Desc.perStageDescriptorSamplerMaxNum = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
    m_Desc.perStageDescriptorConstantBufferMaxNum = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
    m_Desc.perStageDescriptorStorageBufferMaxNum = m_Version >= 1 ? D3D11_1_UAV_SLOT_COUNT : D3D11_PS_CS_UAV_REGISTER_COUNT;
    m_Desc.perStageDescriptorTextureMaxNum = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
    m_Desc.perStageDescriptorStorageTextureMaxNum = m_Version >= 1 ? D3D11_1_UAV_SLOT_COUNT : D3D11_PS_CS_UAV_REGISTER_COUNT;
    m_Desc.perStageResourceMaxNum = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;

    m_Desc.descriptorSetSamplerMaxNum = m_Desc.perStageDescriptorSamplerMaxNum;
    m_Desc.descriptorSetConstantBufferMaxNum = m_Desc.perStageDescriptorConstantBufferMaxNum;
    m_Desc.descriptorSetStorageBufferMaxNum = m_Desc.perStageDescriptorStorageBufferMaxNum;
    m_Desc.descriptorSetTextureMaxNum = m_Desc.perStageDescriptorTextureMaxNum;
    m_Desc.descriptorSetStorageTextureMaxNum = m_Desc.perStageDescriptorStorageTextureMaxNum;

    m_Desc.vertexShaderAttributeMaxNum = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    m_Desc.vertexShaderStreamMaxNum = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    m_Desc.vertexShaderOutputComponentMaxNum = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT * 4;

    m_Desc.tessControlShaderGenerationMaxLevel = D3D11_HS_MAXTESSFACTOR_UPPER_BOUND;
    m_Desc.tessControlShaderPatchPointMaxNum = D3D11_IA_PATCH_MAX_CONTROL_POINT_COUNT;
    m_Desc.tessControlShaderPerVertexInputComponentMaxNum = D3D11_HS_CONTROL_POINT_PHASE_INPUT_REGISTER_COUNT * D3D11_HS_CONTROL_POINT_REGISTER_COMPONENTS;
    m_Desc.tessControlShaderPerVertexOutputComponentMaxNum = D3D11_HS_CONTROL_POINT_PHASE_OUTPUT_REGISTER_COUNT * D3D11_HS_CONTROL_POINT_REGISTER_COMPONENTS;
    m_Desc.tessControlShaderPerPatchOutputComponentMaxNum = D3D11_HS_OUTPUT_PATCH_CONSTANT_REGISTER_SCALAR_COMPONENTS;
    m_Desc.tessControlShaderTotalOutputComponentMaxNum =
        m_Desc.tessControlShaderPatchPointMaxNum * m_Desc.tessControlShaderPerVertexOutputComponentMaxNum + m_Desc.tessControlShaderPerPatchOutputComponentMaxNum;

    m_Desc.tessEvaluationShaderInputComponentMaxNum = D3D11_DS_INPUT_CONTROL_POINT_REGISTER_COUNT * D3D11_DS_INPUT_CONTROL_POINT_REGISTER_COMPONENTS;
    m_Desc.tessEvaluationShaderOutputComponentMaxNum = D3D11_DS_INPUT_CONTROL_POINT_REGISTER_COUNT * D3D11_DS_INPUT_CONTROL_POINT_REGISTER_COMPONENTS;

    m_Desc.geometryShaderInvocationMaxNum = D3D11_GS_MAX_INSTANCE_COUNT;
    m_Desc.geometryShaderInputComponentMaxNum = D3D11_GS_INPUT_REGISTER_COUNT * D3D11_GS_INPUT_REGISTER_COMPONENTS;
    m_Desc.geometryShaderOutputComponentMaxNum = D3D11_GS_OUTPUT_REGISTER_COUNT * D3D11_GS_INPUT_REGISTER_COMPONENTS;
    m_Desc.geometryShaderOutputVertexMaxNum = D3D11_GS_MAX_OUTPUT_VERTEX_COUNT_ACROSS_INSTANCES;
    m_Desc.geometryShaderTotalOutputComponentMaxNum = D3D11_REQ_GS_INVOCATION_32BIT_OUTPUT_COMPONENT_LIMIT;

    m_Desc.fragmentShaderInputComponentMaxNum = D3D11_PS_INPUT_REGISTER_COUNT * D3D11_PS_INPUT_REGISTER_COMPONENTS;
    m_Desc.fragmentShaderOutputAttachmentMaxNum = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
    m_Desc.fragmentShaderDualSourceAttachmentMaxNum = 1;
    m_Desc.fragmentShaderCombinedOutputResourceMaxNum = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT + D3D11_PS_CS_UAV_REGISTER_COUNT;

    m_Desc.computeShaderSharedMemoryMaxSize = D3D11_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
    m_Desc.computeShaderWorkGroupMaxNum[0] = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_Desc.computeShaderWorkGroupMaxNum[1] = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_Desc.computeShaderWorkGroupMaxNum[2] = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_Desc.computeShaderWorkGroupInvocationMaxNum = D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
    m_Desc.computeShaderWorkGroupMaxDim[0] = D3D11_CS_THREAD_GROUP_MAX_X;
    m_Desc.computeShaderWorkGroupMaxDim[1] = D3D11_CS_THREAD_GROUP_MAX_Y;
    m_Desc.computeShaderWorkGroupMaxDim[2] = D3D11_CS_THREAD_GROUP_MAX_Z;

    m_Desc.timestampFrequencyHz = timestampFrequency;
    m_Desc.subPixelPrecisionBits = D3D11_SUBPIXEL_FRACTIONAL_BIT_COUNT;
    m_Desc.subTexelPrecisionBits = D3D11_SUBTEXEL_FRACTIONAL_BIT_COUNT;
    m_Desc.mipmapPrecisionBits = D3D11_MIP_LOD_FRACTIONAL_BIT_COUNT;
    m_Desc.drawIndexedIndex16ValueMax = D3D11_16BIT_INDEX_STRIP_CUT_VALUE;
    m_Desc.drawIndexedIndex32ValueMax = D3D11_32BIT_INDEX_STRIP_CUT_VALUE;
    m_Desc.drawIndirectMaxNum = (1ull << D3D11_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP) - 1;
    m_Desc.samplerLodBiasMin = D3D11_MIP_LOD_BIAS_MIN;
    m_Desc.samplerLodBiasMax = D3D11_MIP_LOD_BIAS_MAX;
    m_Desc.samplerAnisotropyMax = D3D11_DEFAULT_MAX_ANISOTROPY;
    m_Desc.texelOffsetMin = D3D11_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE;
    m_Desc.texelOffsetMax = D3D11_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE;
    m_Desc.texelGatherOffsetMin = D3D11_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE;
    m_Desc.texelGatherOffsetMax = D3D11_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE;
    m_Desc.clipDistanceMaxNum = D3D11_CLIP_OR_CULL_DISTANCE_COUNT;
    m_Desc.cullDistanceMaxNum = D3D11_CLIP_OR_CULL_DISTANCE_COUNT;
    m_Desc.combinedClipAndCullDistanceMaxNum = D3D11_CLIP_OR_CULL_DISTANCE_COUNT;
    m_Desc.conservativeRasterTier = (uint8_t)options2.ConservativeRasterizationTier;

    m_Desc.isTextureFilterMinMaxSupported = options1.MinMaxFiltering != 0;
    m_Desc.isLogicOpSupported = options.OutputMergerLogicOp != 0;
    m_Desc.isDepthBoundsTestSupported = params.extensionsSupported.depthBoundsDeferredContexts;
    m_Desc.isProgrammableSampleLocationsSupported = m_Ext.HasNVAPI();
    m_Desc.isLineSmoothingSupported = true;

    m_Desc.isSwapChainSupported = HasOutput();
    m_Desc.isLowLatencySupported = m_Ext.HasNVAPI();
}

template <typename Implementation, typename Interface, typename... Args>
Result DeviceD3D11::CreateImplementation(Interface*& entity, const Args&... args) {
    Implementation* implementation = Allocate<Implementation>(GetStdAllocator(), *this);
    Result result = implementation->Create(args...);

    if (result == Result::SUCCESS) {
        entity = (Interface*)implementation;
        return Result::SUCCESS;
    }

    Deallocate(GetStdAllocator(), implementation);

    return result;
}

//================================================================================================================
// DeviceBase
//================================================================================================================

void DeviceD3D11::Destroy() {
    Deallocate(GetStdAllocator(), this);
}

//================================================================================================================
// NRI
//================================================================================================================

inline Result DeviceD3D11::CreateSwapChain(const SwapChainDesc& swapChainDesc, SwapChain*& swapChain) {
    return CreateImplementation<SwapChainD3D11>(swapChain, swapChainDesc);
}

inline void DeviceD3D11::DestroySwapChain(SwapChain& swapChain) {
    Deallocate(GetStdAllocator(), (SwapChainD3D11*)&swapChain);
}

inline Result DeviceD3D11::GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue) {
    commandQueue = (CommandQueue*)&m_CommandQueues[(uint32_t)commandQueueType];

    if (commandQueueType != CommandQueueType::GRAPHICS) {
        REPORT_WARNING(this, "%s command queue is not supported by the device!", commandQueueType == CommandQueueType::COMPUTE ? "COMPUTE" : "COPY");
        return Result::UNSUPPORTED;
    }

    return Result::SUCCESS;
}

inline Result DeviceD3D11::CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator) {
    MaybeUnused(commandQueue);

    commandAllocator = (CommandAllocator*)Allocate<CommandAllocatorD3D11>(GetStdAllocator(), *this);

    return Result::SUCCESS;
}

inline Result DeviceD3D11::CreateDescriptorPool(const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    return CreateImplementation<DescriptorPoolD3D11>(descriptorPool, descriptorPoolDesc);
}

inline Result DeviceD3D11::CreateBuffer(const BufferDesc& bufferDesc, Buffer*& buffer) {
    buffer = (Buffer*)Allocate<BufferD3D11>(GetStdAllocator(), *this, bufferDesc);

    return Result::SUCCESS;
}

inline Result DeviceD3D11::CreateTexture(const TextureDesc& textureDesc, Texture*& texture) {
    texture = (Texture*)Allocate<TextureD3D11>(GetStdAllocator(), *this, textureDesc);

    return Result::SUCCESS;
}

inline Result DeviceD3D11::CreateDescriptor(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
    return CreateImplementation<DescriptorD3D11>(bufferView, bufferViewDesc);
}

inline Result DeviceD3D11::CreateDescriptor(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
    return CreateImplementation<DescriptorD3D11>(textureView, textureViewDesc);
}

inline Result DeviceD3D11::CreateDescriptor(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
    return CreateImplementation<DescriptorD3D11>(textureView, textureViewDesc);
}

inline Result DeviceD3D11::CreateDescriptor(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
    return CreateImplementation<DescriptorD3D11>(textureView, textureViewDesc);
}

inline Result DeviceD3D11::CreateDescriptor(const SamplerDesc& samplerDesc, Descriptor*& sampler) {
    return CreateImplementation<DescriptorD3D11>(sampler, samplerDesc);
}

inline Result DeviceD3D11::CreatePipelineLayout(const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout) {
    PipelineLayoutD3D11* implementation = Allocate<PipelineLayoutD3D11>(GetStdAllocator(), *this);
    Result res = implementation->Create(pipelineLayoutDesc);

    if (res == Result::SUCCESS) {
        pipelineLayout = (PipelineLayout*)implementation;
        return Result::SUCCESS;
    }

    Deallocate(GetStdAllocator(), implementation);

    return res;
}

inline Result DeviceD3D11::CreatePipeline(const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline) {
    PipelineD3D11* implementation = Allocate<PipelineD3D11>(GetStdAllocator(), *this);
    Result res = implementation->Create(graphicsPipelineDesc);

    if (res == Result::SUCCESS) {
        pipeline = (Pipeline*)implementation;
        return Result::SUCCESS;
    }

    Deallocate(GetStdAllocator(), implementation);

    return res;
}

inline Result DeviceD3D11::CreatePipeline(const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline) {
    PipelineD3D11* implementation = Allocate<PipelineD3D11>(GetStdAllocator(), *this);
    Result res = implementation->Create(computePipelineDesc);

    if (res == Result::SUCCESS) {
        pipeline = (Pipeline*)implementation;
        return Result::SUCCESS;
    }

    Deallocate(GetStdAllocator(), implementation);

    return res;
}

inline Result DeviceD3D11::CreateQueryPool(const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool) {
    return CreateImplementation<QueryPoolD3D11>(queryPool, queryPoolDesc);
}

inline Result DeviceD3D11::CreateFence(uint64_t initialValue, Fence*& fence) {
    FenceD3D11* implementation = Allocate<FenceD3D11>(GetStdAllocator(), *this);
    Result res = implementation->Create(initialValue);

    if (res == Result::SUCCESS) {
        fence = (Fence*)implementation;
        return Result::SUCCESS;
    }

    Deallocate(GetStdAllocator(), implementation);

    return res;
}

inline void DeviceD3D11::DestroyCommandAllocator(CommandAllocator& commandAllocator) {
    Deallocate(GetStdAllocator(), (CommandAllocatorD3D11*)&commandAllocator);
}

inline void DeviceD3D11::DestroyDescriptorPool(DescriptorPool& descriptorPool) {
    Deallocate(GetStdAllocator(), (DescriptorPoolD3D11*)&descriptorPool);
}

inline void DeviceD3D11::DestroyBuffer(Buffer& buffer) {
    Deallocate(GetStdAllocator(), (BufferD3D11*)&buffer);
}

inline void DeviceD3D11::DestroyTexture(Texture& texture) {
    Deallocate(GetStdAllocator(), (TextureD3D11*)&texture);
}

inline void DeviceD3D11::DestroyDescriptor(Descriptor& descriptor) {
    Deallocate(GetStdAllocator(), (DescriptorD3D11*)&descriptor);
}

inline void DeviceD3D11::DestroyPipelineLayout(PipelineLayout& pipelineLayout) {
    Deallocate(GetStdAllocator(), (PipelineLayoutD3D11*)&pipelineLayout);
}

inline void DeviceD3D11::DestroyPipeline(Pipeline& pipeline) {
    Deallocate(GetStdAllocator(), (PipelineD3D11*)&pipeline);
}

inline void DeviceD3D11::DestroyQueryPool(QueryPool& queryPool) {
    Deallocate(GetStdAllocator(), (QueryPoolD3D11*)&queryPool);
}

inline void DeviceD3D11::DestroyFence(Fence& fence) {
    Deallocate(GetStdAllocator(), (FenceD3D11*)&fence);
}

inline Result DeviceD3D11::AllocateMemory(MemoryType memoryType, uint64_t size, Memory*& memory) {
    MaybeUnused(size);

    memory = (Memory*)Allocate<MemoryD3D11>(GetStdAllocator(), *this, memoryType);

    return Result::SUCCESS;
}

inline Result DeviceD3D11::BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        const BufferMemoryBindingDesc& desc = memoryBindingDescs[i];
        Result res = ((BufferD3D11*)desc.buffer)->Create(*(MemoryD3D11*)desc.memory);
        if (res != Result::SUCCESS)
            return res;
    }

    return Result::SUCCESS;
}

inline Result DeviceD3D11::BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        const TextureMemoryBindingDesc& desc = memoryBindingDescs[i];
        Result res = ((TextureD3D11*)desc.texture)->Create((MemoryD3D11*)desc.memory);
        if (res != Result::SUCCESS)
            return res;
    }

    return Result::SUCCESS;
}

inline void DeviceD3D11::FreeMemory(Memory& memory) {
    Deallocate(GetStdAllocator(), (MemoryD3D11*)&memory);
}

inline FormatSupportBits DeviceD3D11::GetFormatSupport(Format format) const {
    const uint32_t offset = std::min((uint32_t)format, (uint32_t)GetCountOf(D3D_FORMAT_SUPPORT_TABLE) - 1);

    return D3D_FORMAT_SUPPORT_TABLE[offset];
}

inline uint32_t DeviceD3D11::CalculateAllocationNumber(const ResourceGroupDesc& resourceGroupDesc) const {
    HelperDeviceMemoryAllocator allocator(m_CoreInterface, (Device&)*this);

    return allocator.CalculateAllocationNumber(resourceGroupDesc);
}

inline Result DeviceD3D11::AllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations) {
    HelperDeviceMemoryAllocator allocator(m_CoreInterface, (Device&)*this);

    return allocator.AllocateAndBindMemory(resourceGroupDesc, allocations);
}

namespace d3d11 {
#include "D3DExt.hpp"
}

#include "DeviceD3D11.hpp"
