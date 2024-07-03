// © 2021 NVIDIA Corporation

#include "SharedExternal.h"
#include "SharedVal.h"

#include "AccelerationStructureVal.h"
#include "BufferVal.h"
#include "CommandAllocatorVal.h"
#include "CommandBufferVal.h"
#include "CommandQueueVal.h"
#include "DescriptorPoolVal.h"
#include "DescriptorVal.h"
#include "DeviceVal.h"
#include "FenceVal.h"
#include "MemoryVal.h"
#include "PipelineLayoutVal.h"
#include "PipelineVal.h"
#include "QueryPoolVal.h"
#include "SwapChainVal.h"
#include "TextureVal.h"

using namespace nri;

static inline bool IsShaderStageValid(StageBits shaderStages, uint32_t& uniqueShaderStages, StageBits allowedStages) {
    uint32_t x = (uint32_t)(shaderStages & allowedStages);
    uint32_t n = 0;
    while (x) {
        n += x & 1;
        x >>= 1;
    }

    x = (uint32_t)shaderStages;
    bool isUnique = (uniqueShaderStages & x) == 0;
    uniqueShaderStages |= x;

    return n == 1 && isUnique;
}

void ConvertGeometryObjectsVal(GeometryObject* destObjects, const GeometryObject* sourceObjects, uint32_t objectNum);
QueryType GetQueryTypeVK(uint32_t queryTypeVK);

DeviceVal::DeviceVal(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator, DeviceBase& device) :
    DeviceBase(callbacks, stdAllocator), m_Device(*(Device*)&device), m_Name(GetStdAllocator()), m_MemoryTypeMap(GetStdAllocator()) {
}

DeviceVal::~DeviceVal() {
    for (size_t i = 0; i < m_CommandQueues.size(); i++)
        Deallocate(GetStdAllocator(), m_CommandQueues[i]);
    ((DeviceBase*)&m_Device)->Destroy();
}

bool DeviceVal::Create() {
    const DeviceBase& deviceBase = (DeviceBase&)m_Device;

    if (deviceBase.FillFunctionTable(m_CoreAPI) != Result::SUCCESS) {
        REPORT_ERROR(this, "Failed to get 'CoreInterface' interface");
        return false;
    }

    if (deviceBase.FillFunctionTable(m_HelperAPI) != Result::SUCCESS) {
        REPORT_ERROR(this, "Failed to get 'HelperInterface' interface");
        return false;
    }

    if (deviceBase.FillFunctionTable(m_StreamerAPI) != Result::SUCCESS) {
        REPORT_ERROR(this, "Failed to get 'StreamerInterface' interface");
        return false;
    }

    m_IsWrapperD3D11Supported = deviceBase.FillFunctionTable(m_WrapperD3D11API) == Result::SUCCESS;
    m_IsWrapperD3D12Supported = deviceBase.FillFunctionTable(m_WrapperD3D12API) == Result::SUCCESS;
    m_IsWrapperVKSupported = deviceBase.FillFunctionTable(m_WrapperVKAPI) == Result::SUCCESS;
    m_IsSwapChainSupported = deviceBase.FillFunctionTable(m_SwapChainAPI) == Result::SUCCESS;
    m_IsRayTracingSupported = deviceBase.FillFunctionTable(m_RayTracingAPI) == Result::SUCCESS;
    m_IsMeshShaderSupported = deviceBase.FillFunctionTable(m_MeshShaderAPI) == Result::SUCCESS;
    m_IsLowLatencySupported = deviceBase.FillFunctionTable(m_LowLatencyAPI) == Result::SUCCESS;

    return true;
}

void DeviceVal::RegisterMemoryType(MemoryType memoryType, MemoryLocation memoryLocation) {
    ExclusiveScope lockScope(m_Lock);
    m_MemoryTypeMap[memoryType] = memoryLocation;
}

Result DeviceVal::CreateSwapChain(const SwapChainDesc& swapChainDesc, SwapChain*& swapChain) {
    RETURN_ON_FAILURE(this, swapChainDesc.commandQueue != nullptr, Result::INVALID_ARGUMENT, "CreateSwapChain: 'swapChainDesc.commandQueue' is NULL");
    RETURN_ON_FAILURE(this, swapChainDesc.width != 0, Result::INVALID_ARGUMENT, "CreateSwapChain: 'swapChainDesc.width' is 0");
    RETURN_ON_FAILURE(this, swapChainDesc.height != 0, Result::INVALID_ARGUMENT, "CreateSwapChain: 'swapChainDesc.height' is 0");
    RETURN_ON_FAILURE(this, swapChainDesc.textureNum > 0, Result::INVALID_ARGUMENT, "CreateSwapChain: 'swapChainDesc.textureNum' is invalid");
    RETURN_ON_FAILURE(this, swapChainDesc.format < SwapChainFormat::MAX_NUM, Result::INVALID_ARGUMENT, "CreateSwapChain: 'swapChainDesc.format' is invalid");

    auto swapChainDescImpl = swapChainDesc;
    swapChainDescImpl.commandQueue = NRI_GET_IMPL(CommandQueue, swapChainDesc.commandQueue);

    SwapChain* swapChainImpl;
    const Result result = m_SwapChainAPI.CreateSwapChain(m_Device, swapChainDescImpl, swapChainImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, swapChainImpl != nullptr, Result::FAILURE, "CreateSwapChain: 'impl' is NULL");
        swapChain = (SwapChain*)Allocate<SwapChainVal>(GetStdAllocator(), *this, swapChainImpl, swapChainDesc);
    }

    return result;
}

void DeviceVal::DestroySwapChain(SwapChain& swapChain) {
    m_SwapChainAPI.DestroySwapChain(*NRI_GET_IMPL(SwapChain, &swapChain));
    Deallocate(GetStdAllocator(), (SwapChainVal*)&swapChain);
}

void DeviceVal::SetDebugName(const char* name) {
    m_Name = name;
    m_CoreAPI.SetDeviceDebugName(m_Device, name);
}

const DeviceDesc& DeviceVal::GetDesc() const {
    return ((DeviceBase&)m_Device).GetDesc();
}

Result DeviceVal::GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue) {
    RETURN_ON_FAILURE(this, commandQueueType < CommandQueueType::MAX_NUM, Result::INVALID_ARGUMENT, "GetCommandQueue: 'commandQueueType' is invalid");

    CommandQueue* commandQueueImpl;
    const Result result = m_CoreAPI.GetCommandQueue(m_Device, commandQueueType, commandQueueImpl);

    if (result == Result::SUCCESS) {
        const uint32_t index = (uint32_t)commandQueueType;
        if (!m_CommandQueues[index])
            m_CommandQueues[index] = Allocate<CommandQueueVal>(GetStdAllocator(), *this, commandQueueImpl);

        commandQueue = (CommandQueue*)m_CommandQueues[index];
    }

    return result;
}

Result DeviceVal::CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator) {
    auto commandQueueImpl = NRI_GET_IMPL(CommandQueue, &commandQueue);

    CommandAllocator* commandAllocatorImpl = nullptr;
    const Result result = m_CoreAPI.CreateCommandAllocator(*commandQueueImpl, commandAllocatorImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, commandAllocatorImpl != nullptr, Result::FAILURE, "CreateCommandAllocator: 'impl' is NULL");
        commandAllocator = (CommandAllocator*)Allocate<CommandAllocatorVal>(GetStdAllocator(), *this, commandAllocatorImpl);
    }

    return result;
}

Result DeviceVal::CreateDescriptorPool(const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    DescriptorPool* descriptorPoolImpl = nullptr;
    const Result result = m_CoreAPI.CreateDescriptorPool(m_Device, descriptorPoolDesc, descriptorPoolImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, descriptorPoolImpl != nullptr, Result::FAILURE, "CreateDescriptorPool: 'impl' is NULL");
        descriptorPool = (DescriptorPool*)Allocate<DescriptorPoolVal>(GetStdAllocator(), *this, descriptorPoolImpl, descriptorPoolDesc);
    }

    return result;
}

Result DeviceVal::CreateBuffer(const BufferDesc& bufferDesc, Buffer*& buffer) {
    RETURN_ON_FAILURE(this, bufferDesc.size != 0, Result::INVALID_ARGUMENT, "CreateBuffer: 'bufferDesc.size' is 0");

    Buffer* bufferImpl = nullptr;
    const Result result = m_CoreAPI.CreateBuffer(m_Device, bufferDesc, bufferImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, bufferImpl != nullptr, Result::FAILURE, "CreateBuffer: 'impl' is NULL");
        buffer = (Buffer*)Allocate<BufferVal>(GetStdAllocator(), *this, bufferImpl);
    }

    return result;
}

static inline Mip_t GetMaxMipNum(uint16_t w, uint16_t h, uint16_t d) {
    Mip_t mipNum = 1;

    while (w > 1 || h > 1 || d > 1) {
        if (w > 1)
            w >>= 1;

        if (h > 1)
            h >>= 1;

        if (d > 1)
            d >>= 1;

        mipNum++;
    }

    return mipNum;
}

Result DeviceVal::CreateTexture(const TextureDesc& textureDesc, Texture*& texture) {
    Mip_t maxMipNum = GetMaxMipNum(textureDesc.width, textureDesc.height, textureDesc.depth);

    RETURN_ON_FAILURE(
        this, textureDesc.format > Format::UNKNOWN && textureDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT, "CreateTexture: 'textureDesc.format' is invalid");

    RETURN_ON_FAILURE(this, textureDesc.width != 0, Result::INVALID_ARGUMENT, "CreateTexture: 'textureDesc.width' is 0");
    RETURN_ON_FAILURE(this, textureDesc.height != 0, Result::INVALID_ARGUMENT, "CreateTexture: 'textureDesc.height' is 0");
    RETURN_ON_FAILURE(this, textureDesc.depth != 0, Result::INVALID_ARGUMENT, "CreateTexture: 'textureDesc.depth' is 0");
    RETURN_ON_FAILURE(this, textureDesc.mipNum != 0, Result::INVALID_ARGUMENT, "CreateTexture: 'textureDesc.mipNum' is 0");
    RETURN_ON_FAILURE(this, textureDesc.mipNum <= maxMipNum, Result::INVALID_ARGUMENT, "CreateTexture: 'textureDesc.mipNum = %u' can't be > %u", textureDesc.mipNum, maxMipNum);
    RETURN_ON_FAILURE(this, textureDesc.arraySize != 0, Result::INVALID_ARGUMENT, "CreateTexture: 'textureDesc.arraySize' is 0");
    RETURN_ON_FAILURE(this, textureDesc.sampleNum != 0, Result::INVALID_ARGUMENT, "CreateTexture: 'textureDesc.sampleNum' is 0");

    Texture* textureImpl = nullptr;
    const Result result = m_CoreAPI.CreateTexture(m_Device, textureDesc, textureImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, textureImpl != nullptr, Result::FAILURE, "CreateTexture: 'impl' is NULL");
        texture = (Texture*)Allocate<TextureVal>(GetStdAllocator(), *this, textureImpl);
    }

    return result;
}

Result DeviceVal::CreateDescriptor(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
    RETURN_ON_FAILURE(this, bufferViewDesc.buffer != nullptr, Result::INVALID_ARGUMENT, "CreateBufferView: 'bufferViewDesc.buffer' is NULL");
    RETURN_ON_FAILURE(this, bufferViewDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT, "CreateBufferView: 'bufferViewDesc.format' is invalid");
    RETURN_ON_FAILURE(this, bufferViewDesc.viewType < BufferViewType::MAX_NUM, Result::INVALID_ARGUMENT, "CreateBufferView: 'bufferViewDesc.viewType' is invalid");

    const BufferDesc& bufferDesc = ((BufferVal*)bufferViewDesc.buffer)->GetDesc();

    RETURN_ON_FAILURE(this, bufferViewDesc.offset < bufferDesc.size, Result::INVALID_ARGUMENT,
        "CreateBufferView: 'bufferViewDesc.offset' is invalid. (bufferViewDesc.offset=%llu, bufferDesc.size=%llu)", bufferViewDesc.offset, bufferDesc.size);

    RETURN_ON_FAILURE(this, bufferViewDesc.offset + bufferViewDesc.size <= bufferDesc.size, Result::INVALID_ARGUMENT,
        "CreateBufferView: 'bufferViewDesc.size' is invalid. (bufferViewDesc.offset=%llu, bufferViewDesc.size=%llu, bufferDesc.size=%llu)", bufferViewDesc.offset,
        bufferViewDesc.size, bufferDesc.size);

    auto bufferViewDescImpl = bufferViewDesc;
    bufferViewDescImpl.buffer = NRI_GET_IMPL(Buffer, bufferViewDesc.buffer);

    Descriptor* descriptorImpl = nullptr;
    const Result result = m_CoreAPI.CreateBufferView(bufferViewDescImpl, descriptorImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, descriptorImpl != nullptr, Result::FAILURE, "CreateDescriptor: 'impl' is NULL");
        bufferView = (Descriptor*)Allocate<DescriptorVal>(GetStdAllocator(), *this, descriptorImpl, bufferViewDesc);
    }

    return result;
}

Result DeviceVal::CreateDescriptor(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
    RETURN_ON_FAILURE(this, textureViewDesc.texture != nullptr, Result::INVALID_ARGUMENT, "CreateTexture1DView: 'textureViewDesc.texture' is NULL");
    RETURN_ON_FAILURE(this, textureViewDesc.viewType < Texture1DViewType::MAX_NUM, Result::INVALID_ARGUMENT, "CreateTexture1DView: 'textureViewDesc.viewType' is invalid");

    RETURN_ON_FAILURE(this, textureViewDesc.format > Format::UNKNOWN && textureViewDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT,
        "CreateTexture1DView: 'textureViewDesc.format' is invalid");

    const TextureDesc& textureDesc = ((TextureVal*)textureViewDesc.texture)->GetDesc();

    RETURN_ON_FAILURE(this, textureViewDesc.mipOffset < textureDesc.mipNum, Result::INVALID_ARGUMENT,
        "CreateTexture1DView: 'textureViewDesc.mipOffset' is invalid (textureViewDesc.mipOffset=%hu, textureDesc.mipNum=%hu)", textureViewDesc.mipOffset, textureDesc.mipNum);

    RETURN_ON_FAILURE(this, textureViewDesc.mipOffset + textureViewDesc.mipNum <= textureDesc.mipNum, Result::INVALID_ARGUMENT,
        "CreateTexture1DView: 'textureViewDesc.mipNum' is invalid (textureViewDesc.mipOffset=%hu, textureViewDesc.mipNum=%hu, textureDesc.mipNum=%hu)", textureViewDesc.mipOffset,
        textureViewDesc.mipNum, textureDesc.mipNum);

    RETURN_ON_FAILURE(this, textureViewDesc.arrayOffset < textureDesc.arraySize, Result::INVALID_ARGUMENT,
        "CreateTexture1DView: 'textureViewDesc.arrayOffset' is invalid (textureViewDesc.arrayOffset=%hu, textureDesc.arraySize=%hu)", textureViewDesc.arrayOffset,
        textureDesc.arraySize);

    RETURN_ON_FAILURE(this, textureViewDesc.arrayOffset + textureViewDesc.arraySize <= textureDesc.arraySize, Result::INVALID_ARGUMENT,
        "CreateTexture1DView: 'textureViewDesc.arraySize' is invalid (textureViewDesc.arrayOffset=%hu, textureViewDesc.arraySize=%hu, textureDesc.arraySize=%hu)",
        textureViewDesc.arrayOffset, textureViewDesc.arraySize, textureDesc.arraySize);

    auto textureViewDescImpl = textureViewDesc;
    textureViewDescImpl.texture = NRI_GET_IMPL(Texture, textureViewDesc.texture);

    Descriptor* descriptorImpl = nullptr;
    const Result result = m_CoreAPI.CreateTexture1DView(textureViewDescImpl, descriptorImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, descriptorImpl != nullptr, Result::FAILURE, "CreateDescriptor: 'impl' is NULL");
        textureView = (Descriptor*)Allocate<DescriptorVal>(GetStdAllocator(), *this, descriptorImpl, textureViewDesc);
    }

    return result;
}

Result DeviceVal::CreateDescriptor(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
    RETURN_ON_FAILURE(this, textureViewDesc.texture != nullptr, Result::INVALID_ARGUMENT, "CreateTexture2DView: 'textureViewDesc.texture' is NULL");
    RETURN_ON_FAILURE(this, textureViewDesc.viewType < Texture2DViewType::MAX_NUM, Result::INVALID_ARGUMENT, "CreateTexture2DView: 'textureViewDesc.viewType' is invalid");

    RETURN_ON_FAILURE(this, textureViewDesc.format > Format::UNKNOWN && textureViewDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT,
        "CreateTexture2DView: 'textureViewDesc.format' is invalid");

    const TextureDesc& textureDesc = ((TextureVal*)textureViewDesc.texture)->GetDesc();

    RETURN_ON_FAILURE(this, textureViewDesc.mipOffset < textureDesc.mipNum, Result::INVALID_ARGUMENT,
        "CreateTexture2DView: 'textureViewDesc.mipOffset' is invalid. "
        "(textureViewDesc.mipOffset=%hu, textureDesc.mipNum=%hu)",
        textureViewDesc.mipOffset, textureDesc.mipNum);

    RETURN_ON_FAILURE(this, textureViewDesc.mipOffset + textureViewDesc.mipNum <= textureDesc.mipNum, Result::INVALID_ARGUMENT,
        "CreateTexture2DView: 'textureViewDesc.mipNum' is invalid. "
        "(textureViewDesc.mipOffset=%hu, textureViewDesc.mipNum=%hu, textureDesc.mipNum=%hu)",
        textureViewDesc.mipOffset, textureViewDesc.mipNum, textureDesc.mipNum);

    RETURN_ON_FAILURE(this, textureViewDesc.arrayOffset < textureDesc.arraySize, Result::INVALID_ARGUMENT,
        "CreateTexture2DView: 'textureViewDesc.arrayOffset' is invalid. "
        "(textureViewDesc.arrayOffset=%hu, textureDesc.arraySize=%hu)",
        textureViewDesc.arrayOffset, textureDesc.arraySize);

    RETURN_ON_FAILURE(this, textureViewDesc.arrayOffset + textureViewDesc.arraySize <= textureDesc.arraySize, Result::INVALID_ARGUMENT,
        "CreateTexture2DView: 'textureViewDesc.arraySize' is invalid. "
        "(textureViewDesc.arrayOffset=%hu, textureViewDesc.arraySize=%hu, textureDesc.arraySize=%hu)",
        textureViewDesc.arrayOffset, textureViewDesc.arraySize, textureDesc.arraySize);

    auto textureViewDescImpl = textureViewDesc;
    textureViewDescImpl.texture = NRI_GET_IMPL(Texture, textureViewDesc.texture);

    Descriptor* descriptorImpl = nullptr;
    const Result result = m_CoreAPI.CreateTexture2DView(textureViewDescImpl, descriptorImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, descriptorImpl != nullptr, Result::FAILURE, "CreateDescriptor: 'impl' is NULL");
        textureView = (Descriptor*)Allocate<DescriptorVal>(GetStdAllocator(), *this, descriptorImpl, textureViewDesc);
    }

    return result;
}

Result DeviceVal::CreateDescriptor(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
    RETURN_ON_FAILURE(this, textureViewDesc.texture != nullptr, Result::INVALID_ARGUMENT, "CreateTexture3DView: 'textureViewDesc.texture' is NULL");
    RETURN_ON_FAILURE(this, textureViewDesc.viewType < Texture3DViewType::MAX_NUM, Result::INVALID_ARGUMENT, "CreateTexture3DView: 'textureViewDesc.viewType' is invalid");

    RETURN_ON_FAILURE(this, textureViewDesc.format > Format::UNKNOWN && textureViewDesc.format < Format::MAX_NUM, Result::INVALID_ARGUMENT,
        "CreateTexture3DView: 'textureViewDesc.format' is invalid");

    const TextureDesc& textureDesc = ((TextureVal*)textureViewDesc.texture)->GetDesc();

    RETURN_ON_FAILURE(this, textureViewDesc.mipOffset < textureDesc.mipNum, Result::INVALID_ARGUMENT,
        "CreateTexture3DView: 'textureViewDesc.mipOffset' is invalid. "
        "(textureViewDesc.mipOffset=%hu, textureViewDesc.mipOffset=%hu)",
        textureViewDesc.mipOffset, textureDesc.mipNum);

    RETURN_ON_FAILURE(this, textureViewDesc.mipOffset + textureViewDesc.mipNum <= textureDesc.mipNum, Result::INVALID_ARGUMENT,
        "CreateTexture3DView: 'textureViewDesc.mipNum' is invalid. "
        "(textureViewDesc.mipOffset=%hu, textureViewDesc.mipNum=%hu, textureDesc.mipNum=%hu)",
        textureViewDesc.mipOffset, textureViewDesc.mipNum, textureDesc.mipNum);

    RETURN_ON_FAILURE(this, textureViewDesc.sliceOffset < textureDesc.depth, Result::INVALID_ARGUMENT,
        "CreateTexture3DView: 'textureViewDesc.arrayOffset' is invalid. "
        "(textureViewDesc.sliceOffset=%hu, textureDesc.depth=%hu)",
        textureViewDesc.sliceOffset, textureDesc.depth);

    RETURN_ON_FAILURE(this, textureViewDesc.sliceOffset + textureViewDesc.sliceNum <= textureDesc.depth, Result::INVALID_ARGUMENT,
        "CreateTexture3DView: 'textureViewDesc.arraySize' is invalid. "
        "(textureViewDesc.sliceOffset=%hu, textureViewDesc.sliceNum=%hu, textureDesc.depth=%hu)",
        textureViewDesc.sliceOffset, textureViewDesc.sliceNum, textureDesc.depth);

    auto textureViewDescImpl = textureViewDesc;
    textureViewDescImpl.texture = NRI_GET_IMPL(Texture, textureViewDesc.texture);

    Descriptor* descriptorImpl = nullptr;
    const Result result = m_CoreAPI.CreateTexture3DView(textureViewDescImpl, descriptorImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, descriptorImpl != nullptr, Result::FAILURE, "CreateDescriptor: 'impl' is NULL");
        textureView = (Descriptor*)Allocate<DescriptorVal>(GetStdAllocator(), *this, descriptorImpl, textureViewDesc);
    }

    return result;
}

Result DeviceVal::CreateDescriptor(const SamplerDesc& samplerDesc, Descriptor*& sampler) {
    RETURN_ON_FAILURE(this, samplerDesc.filters.mag < Filter::MAX_NUM, Result::INVALID_ARGUMENT, "CreateSampler: 'samplerDesc.filters.mag' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.filters.min < Filter::MAX_NUM, Result::INVALID_ARGUMENT, "CreateSampler: 'samplerDesc.filters.min' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.filters.mip < Filter::MAX_NUM, Result::INVALID_ARGUMENT, "CreateSampler: 'samplerDesc.filters.mip' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.filters.ext < FilterExt::MAX_NUM, Result::INVALID_ARGUMENT, "CreateSampler: 'samplerDesc.filters.ext' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.addressModes.u < AddressMode::MAX_NUM, Result::INVALID_ARGUMENT, "CreateSampler: 'samplerDesc.addressModes.u' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.addressModes.v < AddressMode::MAX_NUM, Result::INVALID_ARGUMENT, "CreateSampler: 'samplerDesc.addressModes.v' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.addressModes.w < AddressMode::MAX_NUM, Result::INVALID_ARGUMENT, "CreateSampler: 'samplerDesc.addressModes.w' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.compareFunc < CompareFunc::MAX_NUM, Result::INVALID_ARGUMENT, "CreateSampler: 'samplerDesc.compareFunc' is invalid");
    RETURN_ON_FAILURE(this, samplerDesc.borderColor < BorderColor::MAX_NUM, Result::INVALID_ARGUMENT, "CreateSampler: 'samplerDesc.borderColor' is invalid");

    Descriptor* samplerImpl = nullptr;
    const Result result = m_CoreAPI.CreateSampler(m_Device, samplerDesc, samplerImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, samplerImpl != nullptr, Result::FAILURE, "CreateDescriptor: 'impl' is NULL");
        sampler = (Descriptor*)Allocate<DescriptorVal>(GetStdAllocator(), *this, samplerImpl);
    }

    return result;
}

Result DeviceVal::CreatePipelineLayout(const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout) {
    const bool isGraphics = pipelineLayoutDesc.shaderStages & StageBits::GRAPHICS_SHADERS;
    const bool isCompute = pipelineLayoutDesc.shaderStages & StageBits::COMPUTE_SHADER;
    const bool isRayTracing = pipelineLayoutDesc.shaderStages & StageBits::RAY_TRACING_SHADERS;
    const uint32_t supportedTypes = (uint32_t)isGraphics + (uint32_t)isCompute + (uint32_t)isRayTracing;

    RETURN_ON_FAILURE(this, pipelineLayoutDesc.shaderStages != StageBits::NONE, Result::INVALID_ARGUMENT, "CreatePipelineLayout: 'pipelineLayoutDesc.shaderStages' can't be NONE");
    RETURN_ON_FAILURE(this, supportedTypes > 0, Result::INVALID_ARGUMENT, "CreatePipelineLayout: 'pipelineLayoutDesc.shaderStages' doesn't include any shader stages");
    RETURN_ON_FAILURE(this, supportedTypes == 1, Result::INVALID_ARGUMENT,
        "CreatePipelineLayout: 'pipelineLayoutDesc.shaderStages' is invalid, it can't be compatible with more than one type of pipeline");

    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        const DescriptorSetDesc& descriptorSetDesc = pipelineLayoutDesc.descriptorSets[i];

        for (uint32_t j = 0; j < descriptorSetDesc.rangeNum; j++) {
            const DescriptorRangeDesc& range = descriptorSetDesc.ranges[j];

            RETURN_ON_FAILURE(this, !range.isDescriptorNumVariable || range.isArray, Result::INVALID_ARGUMENT,
                "CreatePipelineLayout: 'pipelineLayoutDesc.descriptorSets[%u].ranges[%u]' is invalid, "
                "'isArray' can't be false if 'isDescriptorNumVariable' is true",
                i, j);

            RETURN_ON_FAILURE(
                this, range.descriptorNum > 0, Result::INVALID_ARGUMENT, "CreatePipelineLayout: 'pipelineLayoutDesc.descriptorSets[%u].ranges[%u].descriptorNum' is 0", i, j);

            RETURN_ON_FAILURE(this, range.descriptorType < DescriptorType::MAX_NUM, Result::INVALID_ARGUMENT,
                "CreatePipelineLayout: 'pipelineLayoutDesc.descriptorSets[%u].ranges[%u].descriptorType' is invalid", i, j);

            if (range.shaderStages != StageBits::ALL) {
                const uint32_t filteredVisibilityMask = range.shaderStages & pipelineLayoutDesc.shaderStages;

                RETURN_ON_FAILURE(this, (uint32_t)range.shaderStages == filteredVisibilityMask, Result::INVALID_ARGUMENT,
                    "CreatePipelineLayout: 'pipelineLayoutDesc.descriptorSets[%u].ranges[%u].shaderStages' is not "
                    "compatible with 'pipelineLayoutDesc.shaderStages'",
                    i, j);
            }
        }
    }

    PipelineLayout* pipelineLayoutImpl = nullptr;
    const Result result = m_CoreAPI.CreatePipelineLayout(m_Device, pipelineLayoutDesc, pipelineLayoutImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, pipelineLayoutImpl != nullptr, Result::FAILURE, "CreatePipelineLayout: 'impl' is NULL");
        pipelineLayout = (PipelineLayout*)Allocate<PipelineLayoutVal>(GetStdAllocator(), *this, pipelineLayoutImpl, pipelineLayoutDesc);
    }

    return result;
}

Result DeviceVal::CreatePipeline(const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline) {
    RETURN_ON_FAILURE(this, graphicsPipelineDesc.pipelineLayout != nullptr, Result::INVALID_ARGUMENT, "CreatePipeline: 'graphicsPipelineDesc.pipelineLayout' is NULL");
    RETURN_ON_FAILURE(this, graphicsPipelineDesc.shaders != nullptr, Result::INVALID_ARGUMENT, "CreatePipeline: 'graphicsPipelineDesc.shaders' is NULL");
    RETURN_ON_FAILURE(this, graphicsPipelineDesc.shaderNum > 0, Result::INVALID_ARGUMENT, "CreatePipeline: 'graphicsPipelineDesc.shaderNum' is 0");

    const PipelineLayoutVal& pipelineLayout = *(PipelineLayoutVal*)graphicsPipelineDesc.pipelineLayout;
    const StageBits shaderStages = pipelineLayout.GetPipelineLayoutDesc().shaderStages;
    bool hasEntryPoint = false;
    uint32_t uniqueShaderStages = 0;
    for (uint32_t i = 0; i < graphicsPipelineDesc.shaderNum; i++) {
        const ShaderDesc* shaderDesc = graphicsPipelineDesc.shaders + i;
        if (shaderDesc->stage == StageBits::VERTEX_SHADER || shaderDesc->stage == StageBits::MESH_CONTROL_SHADER)
            hasEntryPoint = true;

        RETURN_ON_FAILURE(
            this, shaderDesc->stage & shaderStages, Result::INVALID_ARGUMENT, "CreatePipeline: 'graphicsPipelineDesc.shaders[%u].stage' is not enabled in the pipeline layout", i);
        RETURN_ON_FAILURE(this, shaderDesc->bytecode != nullptr, Result::INVALID_ARGUMENT, "CreatePipeline: 'graphicsPipelineDesc.shaders[%u].bytecode' is invalid", i);
        RETURN_ON_FAILURE(this, shaderDesc->size != 0, Result::INVALID_ARGUMENT, "CreatePipeline: 'graphicsPipelineDesc.shaders[%u].size' is 0", i);
        RETURN_ON_FAILURE(this, IsShaderStageValid(shaderDesc->stage, uniqueShaderStages, StageBits::GRAPHICS_SHADERS), Result::INVALID_ARGUMENT,
            "CreatePipeline: 'graphicsPipelineDesc.shaders[%u].stage' must include only 1 graphics shader stage, unique for the entire pipeline", i);
    }
    RETURN_ON_FAILURE(this, hasEntryPoint, Result::INVALID_ARGUMENT, "CreatePipeline: a VERTEX or MESH_CONTROL shader is not provided");

    for (uint32_t i = 0; i < graphicsPipelineDesc.outputMerger.colorNum; i++) {
        const ColorAttachmentDesc* color = graphicsPipelineDesc.outputMerger.color + i;
        RETURN_ON_FAILURE(this, color->format > Format::UNKNOWN && color->format < Format::BC1_RGBA_UNORM, Result::INVALID_ARGUMENT,
            "CreatePipeline: 'graphicsPipelineDesc.outputMerger->color[%u].format = %u' is invalid", i, color->format);
    }

    if (graphicsPipelineDesc.vertexInput) {
        for (uint32_t i = 0; i < graphicsPipelineDesc.vertexInput->attributeNum; i++) {
            const VertexAttributeDesc* attribute = graphicsPipelineDesc.vertexInput->attributes + i;
            uint32_t size = GetFormatProps(attribute->format).stride;
            uint32_t stride = graphicsPipelineDesc.vertexInput->streams[attribute->streamIndex].stride;
            RETURN_ON_FAILURE(this, attribute->offset + size <= stride, Result::INVALID_ARGUMENT,
                "CreatePipeline: 'graphicsPipelineDesc.inputAssembly->attributes[%u]' is out of bounds of 'graphicsPipelineDesc.inputAssembly->streams[%u]' (stride = %u)", i,
                attribute->streamIndex, stride);
        }
    }

    auto graphicsPipelineDescImpl = graphicsPipelineDesc;
    graphicsPipelineDescImpl.pipelineLayout = NRI_GET_IMPL(PipelineLayout, graphicsPipelineDesc.pipelineLayout);

    Pipeline* pipelineImpl = nullptr;
    const Result result = m_CoreAPI.CreateGraphicsPipeline(m_Device, graphicsPipelineDescImpl, pipelineImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, pipelineImpl != nullptr, Result::FAILURE, "CreatePipeline: 'impl' is NULL");
        pipeline = (Pipeline*)Allocate<PipelineVal>(GetStdAllocator(), *this, pipelineImpl, graphicsPipelineDesc);
    }

    return result;
}

Result DeviceVal::CreatePipeline(const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline) {
    RETURN_ON_FAILURE(this, computePipelineDesc.pipelineLayout != nullptr, Result::INVALID_ARGUMENT, "CreatePipeline: 'computePipelineDesc.pipelineLayout' is NULL");
    RETURN_ON_FAILURE(this, computePipelineDesc.shader.size != 0, Result::INVALID_ARGUMENT, "CreatePipeline: 'computePipelineDesc.shader.size' is 0");
    RETURN_ON_FAILURE(this, computePipelineDesc.shader.bytecode != nullptr, Result::INVALID_ARGUMENT, "CreatePipeline: 'computePipelineDesc.shader.bytecode' is NULL");

    RETURN_ON_FAILURE(this, computePipelineDesc.shader.stage == StageBits::COMPUTE_SHADER, Result::INVALID_ARGUMENT,
        "CreatePipeline: 'computePipelineDesc.shader.stage' must be 'StageBits::COMPUTE_SHADER'");

    auto computePipelineDescImpl = computePipelineDesc;
    computePipelineDescImpl.pipelineLayout = NRI_GET_IMPL(PipelineLayout, computePipelineDesc.pipelineLayout);

    Pipeline* pipelineImpl = nullptr;
    const Result result = m_CoreAPI.CreateComputePipeline(m_Device, computePipelineDescImpl, pipelineImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, pipelineImpl != nullptr, Result::FAILURE, "CreatePipeline: 'impl' is NULL");
        pipeline = (Pipeline*)Allocate<PipelineVal>(GetStdAllocator(), *this, pipelineImpl, computePipelineDesc);
    }

    return result;
}

Result DeviceVal::CreateQueryPool(const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool) {
    RETURN_ON_FAILURE(this, queryPoolDesc.queryType < QueryType::MAX_NUM, Result::INVALID_ARGUMENT, "CreateQueryPool: 'queryPoolDesc.queryType' is invalid");
    RETURN_ON_FAILURE(this, queryPoolDesc.capacity > 0, Result::INVALID_ARGUMENT, "CreateQueryPool: 'queryPoolDesc.capacity' is 0");

    QueryPool* queryPoolImpl = nullptr;
    const Result result = m_CoreAPI.CreateQueryPool(m_Device, queryPoolDesc, queryPoolImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, queryPoolImpl != nullptr, Result::FAILURE, "CreateQueryPool: 'impl' is NULL");
        queryPool = (QueryPool*)Allocate<QueryPoolVal>(GetStdAllocator(), *this, queryPoolImpl, queryPoolDesc.queryType, queryPoolDesc.capacity);
    }

    return result;
}

Result DeviceVal::CreateFence(uint64_t initialValue, Fence*& fence) {
    Fence* fenceImpl;
    const Result result = m_CoreAPI.CreateFence(m_Device, initialValue, fenceImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, fenceImpl != nullptr, Result::FAILURE, "CreateFence: 'impl' is NULL");
        fence = (Fence*)Allocate<FenceVal>(GetStdAllocator(), *this, fenceImpl);
    }

    return result;
}

void DeviceVal::DestroyCommandAllocator(CommandAllocator& commandAllocator) {
    m_CoreAPI.DestroyCommandAllocator(*NRI_GET_IMPL(CommandAllocator, &commandAllocator));
    Deallocate(GetStdAllocator(), (CommandAllocatorVal*)&commandAllocator);
}

void DeviceVal::DestroyDescriptorPool(DescriptorPool& descriptorPool) {
    m_CoreAPI.DestroyDescriptorPool(*NRI_GET_IMPL(DescriptorPool, &descriptorPool));
    Deallocate(GetStdAllocator(), (DescriptorPoolVal*)&descriptorPool);
}

void DeviceVal::DestroyBuffer(Buffer& buffer) {
    m_CoreAPI.DestroyBuffer(*NRI_GET_IMPL(Buffer, &buffer));
    Deallocate(GetStdAllocator(), (BufferVal*)&buffer);
}

void DeviceVal::DestroyTexture(Texture& texture) {
    m_CoreAPI.DestroyTexture(*NRI_GET_IMPL(Texture, &texture));
    Deallocate(GetStdAllocator(), (TextureVal*)&texture);
}

void DeviceVal::DestroyDescriptor(Descriptor& descriptor) {
    m_CoreAPI.DestroyDescriptor(*NRI_GET_IMPL(Descriptor, &descriptor));
    Deallocate(GetStdAllocator(), (DescriptorVal*)&descriptor);
}

void DeviceVal::DestroyPipelineLayout(PipelineLayout& pipelineLayout) {
    m_CoreAPI.DestroyPipelineLayout(*NRI_GET_IMPL(PipelineLayout, &pipelineLayout));
    Deallocate(GetStdAllocator(), (PipelineLayoutVal*)&pipelineLayout);
}

void DeviceVal::DestroyPipeline(Pipeline& pipeline) {
    m_CoreAPI.DestroyPipeline(*NRI_GET_IMPL(Pipeline, &pipeline));
    Deallocate(GetStdAllocator(), (PipelineVal*)&pipeline);
}

void DeviceVal::DestroyQueryPool(QueryPool& queryPool) {
    m_CoreAPI.DestroyQueryPool(*NRI_GET_IMPL(QueryPool, &queryPool));
    Deallocate(GetStdAllocator(), (QueryPoolVal*)&queryPool);
}

void DeviceVal::DestroyFence(Fence& fence) {
    m_CoreAPI.DestroyFence(*NRI_GET_IMPL(Fence, &fence));
    Deallocate(GetStdAllocator(), (FenceVal*)&fence);
}

Result DeviceVal::AllocateMemory(MemoryType memoryType, uint64_t size, Memory*& memory) {
    RETURN_ON_FAILURE(this, size > 0, Result::INVALID_ARGUMENT, "AllocateMemory: 'size' is 0");

    std::unordered_map<MemoryType, MemoryLocation>::iterator it;
    std::unordered_map<MemoryType, MemoryLocation>::iterator end;
    {
        ExclusiveScope lockScope(m_Lock);
        it = m_MemoryTypeMap.find(memoryType);
        end = m_MemoryTypeMap.end();
    }

    RETURN_ON_FAILURE(this, it != end, Result::FAILURE, "AllocateMemory: 'memoryType' is invalid");

    Memory* memoryImpl;
    const Result result = m_CoreAPI.AllocateMemory(m_Device, memoryType, size, memoryImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, memoryImpl != nullptr, Result::FAILURE, "AllocateMemory: 'impl' is NULL");
        memory = (Memory*)Allocate<MemoryVal>(GetStdAllocator(), *this, memoryImpl, size, it->second);
    }

    return result;
}

Result DeviceVal::BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    if (memoryBindingDescNum == 0)
        return Result::SUCCESS;

    RETURN_ON_FAILURE(this, memoryBindingDescs != nullptr, Result::INVALID_ARGUMENT, "BindBufferMemory: 'memoryBindingDescs' is NULL");

    BufferMemoryBindingDesc* memoryBindingDescsImpl = STACK_ALLOC(BufferMemoryBindingDesc, memoryBindingDescNum);

    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        BufferMemoryBindingDesc& destDesc = memoryBindingDescsImpl[i];
        const BufferMemoryBindingDesc& srcDesc = memoryBindingDescs[i];

        RETURN_ON_FAILURE(this, srcDesc.buffer != nullptr, Result::INVALID_ARGUMENT, "BindBufferMemory: 'memoryBindingDescs[%u].buffer' is NULL", i);
        RETURN_ON_FAILURE(this, srcDesc.memory != nullptr, Result::INVALID_ARGUMENT, "BindBufferMemory: 'memoryBindingDescs[%u].memory' is NULL", i);

        MemoryVal& memory = (MemoryVal&)*srcDesc.memory;
        BufferVal& buffer = (BufferVal&)*srcDesc.buffer;

        RETURN_ON_FAILURE(this, !buffer.IsBoundToMemory(), Result::INVALID_ARGUMENT, "BindBufferMemory: 'memoryBindingDescs[%u].buffer' is already bound to memory", i);

        destDesc = srcDesc;
        destDesc.memory = memory.GetImpl();
        destDesc.buffer = buffer.GetImpl();

        // Skip validation if memory has been created from GAPI object using a wrapper extension
        if (memory.GetMemoryLocation() == MemoryLocation::MAX_NUM)
            continue;

        MemoryDesc memoryDesc = {};
        buffer.GetMemoryInfo(memory.GetMemoryLocation(), memoryDesc);

        RETURN_ON_FAILURE(this, !memoryDesc.mustBeDedicated || srcDesc.offset == 0, Result::INVALID_ARGUMENT,
            "BindBufferMemory: 'memoryBindingDescs[%u].offset' must be zero for dedicated allocation", i);

        RETURN_ON_FAILURE(this, memoryDesc.alignment != 0, Result::INVALID_ARGUMENT, "BindBufferMemory: 'memoryBindingDescs[%u].alignment' is 0", i);
        RETURN_ON_FAILURE(this, srcDesc.offset % memoryDesc.alignment == 0, Result::INVALID_ARGUMENT, "BindBufferMemory: 'memoryBindingDescs[%u].offset' is misaligned", i);

        const uint64_t rangeMax = srcDesc.offset + memoryDesc.size;
        const bool memorySizeIsUnknown = memory.GetSize() == 0;

        RETURN_ON_FAILURE(this, memorySizeIsUnknown || rangeMax <= memory.GetSize(), Result::INVALID_ARGUMENT, "BindBufferMemory: 'memoryBindingDescs[%u].offset' is invalid", i);
    }

    const Result result = m_CoreAPI.BindBufferMemory(m_Device, memoryBindingDescsImpl, memoryBindingDescNum);

    if (result == Result::SUCCESS) {
        for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
            MemoryVal& memory = *(MemoryVal*)memoryBindingDescs[i].memory;
            memory.BindBuffer(*(BufferVal*)memoryBindingDescs[i].buffer);
        }
    }

    return result;
}

Result DeviceVal::BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    RETURN_ON_FAILURE(this, memoryBindingDescs != nullptr || memoryBindingDescNum == 0, Result::INVALID_ARGUMENT, "BindTextureMemory: 'memoryBindingDescs' is a NULL");

    TextureMemoryBindingDesc* memoryBindingDescsImpl = STACK_ALLOC(TextureMemoryBindingDesc, memoryBindingDescNum);

    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        TextureMemoryBindingDesc& destDesc = memoryBindingDescsImpl[i];
        const TextureMemoryBindingDesc& srcDesc = memoryBindingDescs[i];

        RETURN_ON_FAILURE(this, srcDesc.texture != nullptr, Result::INVALID_ARGUMENT, "BindTextureMemory: 'memoryBindingDescs[%u].texture' is NULL", i);
        RETURN_ON_FAILURE(this, srcDesc.memory != nullptr, Result::INVALID_ARGUMENT, "BindTextureMemory: 'memoryBindingDescs[%u].memory' is NULL", i);

        MemoryVal& memory = (MemoryVal&)*srcDesc.memory;
        TextureVal& texture = (TextureVal&)*srcDesc.texture;

        RETURN_ON_FAILURE(this, !texture.IsBoundToMemory(), Result::INVALID_ARGUMENT, "BindTextureMemory: 'memoryBindingDescs[%u].texture' is already bound to memory", i);

        destDesc = srcDesc;
        destDesc.memory = memory.GetImpl();
        destDesc.texture = texture.GetImpl();

        // Skip validation if memory has been created from GAPI object using a wrapper extension
        if (memory.GetMemoryLocation() == MemoryLocation::MAX_NUM)
            continue;

        MemoryDesc memoryDesc = {};
        texture.GetMemoryInfo(memory.GetMemoryLocation(), memoryDesc);

        RETURN_ON_FAILURE(this, !memoryDesc.mustBeDedicated || srcDesc.offset == 0, Result::INVALID_ARGUMENT,
            "BindTextureMemory: 'memoryBindingDescs[%u].offset' must be zero for dedicated allocation", i);

        RETURN_ON_FAILURE(this, memoryDesc.alignment != 0, Result::INVALID_ARGUMENT, "BindTextureMemory: 'memoryBindingDescs[%u].alignment' is 0", i);
        RETURN_ON_FAILURE(this, srcDesc.offset % memoryDesc.alignment == 0, Result::INVALID_ARGUMENT, "BindTextureMemory: 'memoryBindingDescs[%u].offset' is misaligned", i);

        const uint64_t rangeMax = srcDesc.offset + memoryDesc.size;
        const bool memorySizeIsUnknown = memory.GetSize() == 0;

        RETURN_ON_FAILURE(this, memorySizeIsUnknown || rangeMax <= memory.GetSize(), Result::INVALID_ARGUMENT, "BindTextureMemory: 'memoryBindingDescs[%u].offset' is invalid", i);
    }

    const Result result = m_CoreAPI.BindTextureMemory(m_Device, memoryBindingDescsImpl, memoryBindingDescNum);

    if (result == Result::SUCCESS) {
        for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
            MemoryVal& memory = *(MemoryVal*)memoryBindingDescs[i].memory;
            memory.BindTexture(*(TextureVal*)memoryBindingDescs[i].texture);
        }
    }

    return result;
}

void DeviceVal::FreeMemory(Memory& memory) {
    MemoryVal& memoryVal = (MemoryVal&)memory;

    if (memoryVal.HasBoundResources()) {
        memoryVal.ReportBoundResources();
        REPORT_ERROR(this, "FreeMemory: some resources are still bound to the memory");
        return;
    }

    m_CoreAPI.FreeMemory(*NRI_GET_IMPL(Memory, &memory));
    Deallocate(GetStdAllocator(), (MemoryVal*)&memory);
}

FormatSupportBits DeviceVal::GetFormatSupport(Format format) const {
    return m_CoreAPI.GetFormatSupport(m_Device, format);
}

#if NRI_USE_VULKAN

Result DeviceVal::CreateCommandQueueVK(const CommandQueueVKDesc& commandQueueVKDesc, CommandQueue*& commandQueue) {
    RETURN_ON_FAILURE(this, commandQueueVKDesc.vkQueue != 0, Result::INVALID_ARGUMENT, "CreateCommandQueueVK: 'commandQueueVKDesc.vkQueue' is NULL");

    RETURN_ON_FAILURE(
        this, commandQueueVKDesc.commandQueueType < CommandQueueType::MAX_NUM, Result::INVALID_ARGUMENT, "CreateCommandQueueVK: 'commandQueueVKDesc.commandQueueType' is invalid");

    CommandQueue* commandQueueImpl = nullptr;
    const Result result = m_WrapperVKAPI.CreateCommandQueueVK(m_Device, commandQueueVKDesc, commandQueueImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, commandQueueImpl != nullptr, Result::FAILURE, "CreateCommandQueueVK: 'impl' is NULL");

        commandQueue = (CommandQueue*)Allocate<CommandQueueVal>(GetStdAllocator(), *this, commandQueueImpl);
    }

    return result;
}

Result DeviceVal::CreateCommandAllocatorVK(const CommandAllocatorVKDesc& commandAllocatorVKDesc, CommandAllocator*& commandAllocator) {
    RETURN_ON_FAILURE(this, commandAllocatorVKDesc.vkCommandPool != 0, Result::INVALID_ARGUMENT, "CreateCommandAllocatorVK: 'commandAllocatorVKDesc.vkCommandPool' is NULL");

    RETURN_ON_FAILURE(this, commandAllocatorVKDesc.commandQueueType < CommandQueueType::MAX_NUM, Result::INVALID_ARGUMENT,
        "CreateCommandAllocatorVK: 'commandAllocatorVKDesc.commandQueueType' is invalid");

    CommandAllocator* commandAllocatorImpl = nullptr;
    const Result result = m_WrapperVKAPI.CreateCommandAllocatorVK(m_Device, commandAllocatorVKDesc, commandAllocatorImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, commandAllocatorImpl != nullptr, Result::FAILURE, "CreateCommandAllocatorVK: 'impl' is NULL");

        commandAllocator = (CommandAllocator*)Allocate<CommandAllocatorVal>(GetStdAllocator(), *this, commandAllocatorImpl);
    }

    return result;
}

Result DeviceVal::CreateCommandBufferVK(const CommandBufferVKDesc& commandBufferVKDesc, CommandBuffer*& commandBuffer) {
    RETURN_ON_FAILURE(this, commandBufferVKDesc.vkCommandBuffer != 0, Result::INVALID_ARGUMENT, "CreateCommandBufferVK: 'commandBufferVKDesc.vkCommandBuffer' is NULL");

    RETURN_ON_FAILURE(this, commandBufferVKDesc.commandQueueType < CommandQueueType::MAX_NUM, Result::INVALID_ARGUMENT,
        "CreateCommandBufferVK: 'commandBufferVKDesc.commandQueueType' is invalid");

    CommandBuffer* commandBufferImpl = nullptr;
    const Result result = m_WrapperVKAPI.CreateCommandBufferVK(m_Device, commandBufferVKDesc, commandBufferImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, commandBufferImpl != nullptr, Result::FAILURE, "CreateCommandBufferVK: 'impl' is NULL");

        commandBuffer = (CommandBuffer*)Allocate<CommandBufferVal>(GetStdAllocator(), *this, commandBufferImpl, true);
    }

    return result;
}

Result DeviceVal::CreateDescriptorPoolVK(const DescriptorPoolVKDesc& descriptorPoolVKDesc, DescriptorPool*& descriptorPool) {
    RETURN_ON_FAILURE(this, descriptorPoolVKDesc.vkDescriptorPool != 0, Result::INVALID_ARGUMENT, "CreateDescriptorPoolVK: 'vkDescriptorPool' is NULL");
    RETURN_ON_FAILURE(this, descriptorPoolVKDesc.descriptorSetMaxNum != 0, Result::INVALID_ARGUMENT, "CreateDescriptorPoolVK: 'descriptorSetMaxNum' is 0");

    DescriptorPool* descriptorPoolImpl = nullptr;
    const Result result = m_WrapperVKAPI.CreateDescriptorPoolVK(m_Device, descriptorPoolVKDesc, descriptorPoolImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, descriptorPoolImpl != nullptr, Result::FAILURE, "CreateDescriptorPoolVK: 'impl' is NULL");

        descriptorPool = (DescriptorPool*)Allocate<DescriptorPoolVal>(GetStdAllocator(), *this, descriptorPoolImpl, descriptorPoolVKDesc.descriptorSetMaxNum);
    }

    return result;
}

Result DeviceVal::CreateBufferVK(const BufferVKDesc& bufferDesc, Buffer*& buffer) {
    RETURN_ON_FAILURE(this, bufferDesc.vkBuffer != 0, Result::INVALID_ARGUMENT, "CreateBufferVK: 'bufferDesc.vkBuffer' is NULL");
    RETURN_ON_FAILURE(this, bufferDesc.memory != nullptr, Result::INVALID_ARGUMENT, "CreateBufferVK: 'bufferDesc.memory' is NULL");
    RETURN_ON_FAILURE(this, bufferDesc.size > 0, Result::INVALID_ARGUMENT, "CreateBufferVK: 'bufferDesc.bufferSize' is 0");

    Buffer* bufferImpl = nullptr;
    const Result result = m_WrapperVKAPI.CreateBufferVK(m_Device, bufferDesc, bufferImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, bufferImpl != nullptr, Result::FAILURE, "CreateBufferVK: 'impl' is NULL");

        buffer = (Buffer*)Allocate<BufferVal>(GetStdAllocator(), *this, bufferImpl);
    }

    return result;
}

Result DeviceVal::CreateTextureVK(const TextureVKDesc& textureVKDesc, Texture*& texture) {
    RETURN_ON_FAILURE(this, textureVKDesc.vkImage != 0, Result::INVALID_ARGUMENT, "CreateTextureVK: 'textureVKDesc.vkImage' is NULL");
    RETURN_ON_FAILURE(this, nriConvertVKFormatToNRI(textureVKDesc.vkFormat) != Format::UNKNOWN, Result::INVALID_ARGUMENT, "CreateTextureVK: 'textureVKDesc.sampleNum' is 0");
    RETURN_ON_FAILURE(this, textureVKDesc.sampleNum > 0, Result::INVALID_ARGUMENT, "CreateTextureVK: 'textureVKDesc.sampleNum' is 0");
    RETURN_ON_FAILURE(this, textureVKDesc.arraySize > 0, Result::INVALID_ARGUMENT, "CreateTextureVK: 'textureVKDesc.arraySize' is 0");
    RETURN_ON_FAILURE(this, textureVKDesc.mipNum > 0, Result::INVALID_ARGUMENT, "CreateTextureVK: 'textureVKDesc.mipNum' is 0");

    Texture* textureImpl = nullptr;
    const Result result = m_WrapperVKAPI.CreateTextureVK(m_Device, textureVKDesc, textureImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, textureImpl != nullptr, Result::FAILURE, "CreateTextureVK: 'impl' is NULL");

        texture = (Texture*)Allocate<TextureVal>(GetStdAllocator(), *this, textureImpl);
    }

    return result;
}

Result DeviceVal::CreateMemoryVK(const MemoryVKDesc& memoryVKDesc, Memory*& memory) {
    RETURN_ON_FAILURE(this, memoryVKDesc.vkDeviceMemory != 0, Result::INVALID_ARGUMENT, "CreateMemoryVK: 'memoryVKDesc.vkDeviceMemory' is NULL");
    RETURN_ON_FAILURE(this, memoryVKDesc.size > 0, Result::INVALID_ARGUMENT, "CreateMemoryVK: 'memoryVKDesc.size' is 0");

    Memory* memoryImpl = nullptr;
    const Result result = m_WrapperVKAPI.CreateMemoryVK(m_Device, memoryVKDesc, memoryImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, memoryImpl != nullptr, Result::FAILURE, "CreateMemoryVK: 'impl' is NULL");

        memory = (Memory*)Allocate<MemoryVal>(GetStdAllocator(), *this, memoryImpl, memoryVKDesc.size, MemoryLocation::MAX_NUM);
    }

    return result;
}

Result DeviceVal::CreateGraphicsPipelineVK(NRIVkPipeline vkPipeline, Pipeline*& pipeline) {
    RETURN_ON_FAILURE(this, vkPipeline != 0, Result::INVALID_ARGUMENT, "CreateGraphicsPipelineVK: 'vkPipeline' is NULL");

    Pipeline* pipelineImpl = nullptr;
    const Result result = m_WrapperVKAPI.CreateGraphicsPipelineVK(m_Device, vkPipeline, pipelineImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, pipelineImpl != nullptr, Result::FAILURE, "CreateGraphicsPipelineVK: 'impl' is NULL");

        pipeline = (Pipeline*)Allocate<PipelineVal>(GetStdAllocator(), *this, pipelineImpl);
    }

    return result;
}

Result DeviceVal::CreateComputePipelineVK(NRIVkPipeline vkPipeline, Pipeline*& pipeline) {
    RETURN_ON_FAILURE(this, vkPipeline != 0, Result::INVALID_ARGUMENT, "CreateComputePipelineVK: 'vkPipeline' is NULL");

    Pipeline* pipelineImpl = nullptr;
    const Result result = m_WrapperVKAPI.CreateComputePipelineVK(m_Device, vkPipeline, pipelineImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, pipelineImpl != nullptr, Result::FAILURE, "CreateComputePipelineVK: 'impl' is NULL");

        pipeline = (Pipeline*)Allocate<PipelineVal>(GetStdAllocator(), *this, pipelineImpl);
    }

    return result;
}

Result DeviceVal::CreateQueryPoolVK(const QueryPoolVKDesc& queryPoolVKDesc, QueryPool*& queryPool) {
    RETURN_ON_FAILURE(this, queryPoolVKDesc.vkQueryPool != 0, Result::INVALID_ARGUMENT, "CreateQueryPoolVK: 'queryPoolVKDesc.vkQueryPool' is NULL");

    QueryPool* queryPoolImpl = nullptr;
    const Result result = m_WrapperVKAPI.CreateQueryPoolVK(m_Device, queryPoolVKDesc, queryPoolImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, queryPoolImpl != nullptr, Result::FAILURE, "CreateQueryPoolVK: 'impl' is NULL");

        const QueryType queryType = GetQueryTypeVK(queryPoolVKDesc.vkQueryType);

        queryPool = (QueryPool*)Allocate<QueryPoolVal>(GetStdAllocator(), *this, queryPoolImpl, queryType, 0);
    }

    return result;
}

Result DeviceVal::CreateAccelerationStructureVK(const AccelerationStructureVKDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    RETURN_ON_FAILURE(this, accelerationStructureDesc.vkAccelerationStructure != 0, Result::INVALID_ARGUMENT,
        "CreateAccelerationStructureVK: 'accelerationStructureDesc.vkAccelerationStructure' is NULL");

    AccelerationStructure* accelerationStructureImpl = nullptr;
    const Result result = m_WrapperVKAPI.CreateAccelerationStructureVK(m_Device, accelerationStructureDesc, accelerationStructureImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, accelerationStructureImpl != nullptr, Result::FAILURE, "CreateAccelerationStructureVK: 'impl' is NULL");

        accelerationStructure = (AccelerationStructure*)Allocate<AccelerationStructureVal>(GetStdAllocator(), *this, accelerationStructureImpl, true);
    }

    return result;
}

#endif

#if NRI_USE_D3D11

Result DeviceVal::CreateCommandBufferD3D11(const CommandBufferD3D11Desc& commandBufferDesc, CommandBuffer*& commandBuffer) {
    RETURN_ON_FAILURE(this, commandBufferDesc.d3d11DeviceContext != nullptr, Result::INVALID_ARGUMENT, "CreateCommandBufferD3D11: 'commandBufferDesc.d3d11DeviceContext' is NULL");

    CommandBuffer* commandBufferImpl = nullptr;
    const Result result = m_WrapperD3D11API.CreateCommandBufferD3D11(m_Device, commandBufferDesc, commandBufferImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, commandBufferImpl != nullptr, Result::FAILURE, "CreateCommandBufferD3D11: 'impl' is NULL");

        commandBuffer = (CommandBuffer*)Allocate<CommandBufferVal>(GetStdAllocator(), *this, commandBufferImpl, true);
    }

    return result;
}

Result DeviceVal::CreateBufferD3D11(const BufferD3D11Desc& bufferDesc, Buffer*& buffer) {
    RETURN_ON_FAILURE(this, bufferDesc.d3d11Resource != nullptr, Result::INVALID_ARGUMENT, "CreateBufferD3D11: 'bufferDesc.d3d11Resource' is NULL");
    RETURN_ON_FAILURE(this, bufferDesc.bufferDesc != nullptr, Result::INVALID_ARGUMENT, "CreateBufferD3D11: 'bufferDesc.bufferDesc' is NULL");

    Buffer* bufferImpl = nullptr;
    const Result result = m_WrapperD3D11API.CreateBufferD3D11(m_Device, bufferDesc, bufferImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, bufferImpl != nullptr, Result::FAILURE, "CreateBufferD3D11: 'impl' is NULL");

        buffer = (Buffer*)Allocate<BufferVal>(GetStdAllocator(), *this, bufferImpl);
    }

    return result;
}

Result DeviceVal::CreateTextureD3D11(const TextureD3D11Desc& textureDesc, Texture*& texture) {
    RETURN_ON_FAILURE(this, textureDesc.d3d11Resource != nullptr, Result::INVALID_ARGUMENT, "CreateTextureD3D11: 'textureDesc.d3d11Resource' is NULL");

    Texture* textureImpl = nullptr;
    const Result result = m_WrapperD3D11API.CreateTextureD3D11(m_Device, textureDesc, textureImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, textureImpl != nullptr, Result::FAILURE, "CreateTextureD3D11: 'impl' is NULL");

        texture = (Texture*)Allocate<TextureVal>(GetStdAllocator(), *this, textureImpl);
    }

    return result;
}

#endif

#if NRI_USE_D3D12

Result DeviceVal::CreateCommandBufferD3D12(const CommandBufferD3D12Desc& commandBufferDesc, CommandBuffer*& commandBuffer) {
    RETURN_ON_FAILURE(
        this, commandBufferDesc.d3d12CommandAllocator != nullptr, Result::INVALID_ARGUMENT, "CreateCommandBufferD3D12: 'commandBufferDesc.d3d12CommandAllocator' is NULL");

    RETURN_ON_FAILURE(this, commandBufferDesc.d3d12CommandList != nullptr, Result::INVALID_ARGUMENT, "CreateCommandBufferD3D12: 'commandBufferDesc.d3d12CommandList' is NULL");

    CommandBuffer* commandBufferImpl = nullptr;
    const Result result = m_WrapperD3D12API.CreateCommandBufferD3D12(m_Device, commandBufferDesc, commandBufferImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, commandBufferImpl != nullptr, Result::FAILURE, "CreateCommandBufferD3D12: 'impl' is NULL");

        commandBuffer = (CommandBuffer*)Allocate<CommandBufferVal>(GetStdAllocator(), *this, commandBufferImpl, true);
    }

    return result;
}

Result DeviceVal::CreateDescriptorPoolD3D12(const DescriptorPoolD3D12Desc& descriptorPoolD3D12Desc, DescriptorPool*& descriptorPool) {
    RETURN_ON_FAILURE(this, descriptorPoolD3D12Desc.d3d12ResourceDescriptorHeap == nullptr && descriptorPoolD3D12Desc.d3d12ResourceDescriptorHeap == nullptr,
        Result::INVALID_ARGUMENT,
        "CreateDescriptorPoolD3D12: 'descriptorPoolD3D12Desc.d3d12ResourceDescriptorHeap' and 'descriptorPoolD3D12Desc.d3d12ResourceDescriptorHeap' are NULL");

    DescriptorPool* descriptorPoolImpl = nullptr;
    const Result result = m_WrapperD3D12API.CreateDescriptorPoolD3D12(m_Device, descriptorPoolD3D12Desc, descriptorPoolImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, descriptorPoolImpl != nullptr, Result::FAILURE, "CreateDescriptorPoolD3D12: 'impl' is NULL");

        descriptorPool = (DescriptorPool*)Allocate<DescriptorPoolVal>(GetStdAllocator(), *this, descriptorPoolImpl, descriptorPoolD3D12Desc.descriptorSetMaxNum);
    }

    return result;
}

Result DeviceVal::CreateBufferD3D12(const BufferD3D12Desc& bufferDesc, Buffer*& buffer) {
    RETURN_ON_FAILURE(this, bufferDesc.d3d12Resource != nullptr, Result::INVALID_ARGUMENT, "CreateBufferD3D12: 'bufferDesc.d3d12Resource' is NULL");
    RETURN_ON_FAILURE(this, bufferDesc.bufferDesc != nullptr, Result::INVALID_ARGUMENT, "CreateBufferD3D12: 'bufferDesc.bufferDesc' is NULL");

    Buffer* bufferImpl = nullptr;
    const Result result = m_WrapperD3D12API.CreateBufferD3D12(m_Device, bufferDesc, bufferImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, bufferImpl != nullptr, Result::FAILURE, "CreateBufferD3D12: 'impl' is NULL");

        buffer = (Buffer*)Allocate<BufferVal>(GetStdAllocator(), *this, bufferImpl);
    }

    return result;
}

Result DeviceVal::CreateTextureD3D12(const TextureD3D12Desc& textureDesc, Texture*& texture) {
    RETURN_ON_FAILURE(this, textureDesc.d3d12Resource != nullptr, Result::INVALID_ARGUMENT, "CreateTextureD3D12: 'textureDesc.d3d12Resource' is NULL");

    Texture* textureImpl = nullptr;
    const Result result = m_WrapperD3D12API.CreateTextureD3D12(m_Device, textureDesc, textureImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, textureImpl != nullptr, Result::FAILURE, "CreateTextureD3D12: 'impl' is NULL");

        texture = (Texture*)Allocate<TextureVal>(GetStdAllocator(), *this, textureImpl);
    }

    return result;
}

Result DeviceVal::CreateMemoryD3D12(const MemoryD3D12Desc& memoryDesc, Memory*& memory) {
    RETURN_ON_FAILURE(this, (memoryDesc.d3d12Heap != nullptr || memoryDesc.d3d12HeapDesc != nullptr), Result::INVALID_ARGUMENT,
        "CreateMemoryD3D12: 'memoryDesc.d3d12Heap' or 'memoryDesc.d3d12HeapDesc' is NULL");

    Memory* memoryImpl = nullptr;
    const Result result = m_WrapperD3D12API.CreateMemoryD3D12(m_Device, memoryDesc, memoryImpl);

    const uint64_t size = GetMemorySizeD3D12(memoryDesc);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, memoryImpl != nullptr, Result::FAILURE, "CreateMemoryD3D12: 'impl' is NULL");

        memory = (Memory*)Allocate<MemoryVal>(GetStdAllocator(), *this, memoryImpl, size, MemoryLocation::MAX_NUM);
    }

    return result;
}

Result DeviceVal::CreateAccelerationStructureD3D12(const AccelerationStructureD3D12Desc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    RETURN_ON_FAILURE(
        this, accelerationStructureDesc.d3d12Resource != nullptr, Result::INVALID_ARGUMENT, "CreateAccelerationStructureD3D12: 'accelerationStructureDesc.d3d12Resource' is NULL");

    AccelerationStructure* accelerationStructureImpl = nullptr;
    const Result result = m_WrapperD3D12API.CreateAccelerationStructureD3D12(m_Device, accelerationStructureDesc, accelerationStructureImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, accelerationStructureImpl != nullptr, Result::FAILURE, "CreateAccelerationStructureD3D12: 'impl' is NULL");

        accelerationStructure = (AccelerationStructure*)Allocate<AccelerationStructureVal>(GetStdAllocator(), *this, accelerationStructureImpl, true);
    }

    return result;
}

#endif

uint32_t DeviceVal::CalculateAllocationNumber(const ResourceGroupDesc& resourceGroupDesc) const {
    RETURN_ON_FAILURE(this, resourceGroupDesc.memoryLocation < MemoryLocation::MAX_NUM, 0, "CalculateAllocationNumber: 'resourceGroupDesc.memoryLocation' is invalid");
    RETURN_ON_FAILURE(this, resourceGroupDesc.bufferNum == 0 || resourceGroupDesc.buffers != nullptr, 0, "CalculateAllocationNumber: 'resourceGroupDesc.buffers' is NULL");
    RETURN_ON_FAILURE(this, resourceGroupDesc.textureNum == 0 || resourceGroupDesc.textures != nullptr, 0, "CalculateAllocationNumber: 'resourceGroupDesc.textures' is NULL");

    Buffer** buffersImpl = STACK_ALLOC(Buffer*, resourceGroupDesc.bufferNum);

    for (uint32_t i = 0; i < resourceGroupDesc.bufferNum; i++) {
        RETURN_ON_FAILURE(this, resourceGroupDesc.buffers[i] != nullptr, 0, "CalculateAllocationNumber: 'resourceGroupDesc.buffers[%u]' is NULL", i);

        BufferVal& bufferVal = *(BufferVal*)resourceGroupDesc.buffers[i];
        buffersImpl[i] = bufferVal.GetImpl();
    }

    Texture** texturesImpl = STACK_ALLOC(Texture*, resourceGroupDesc.textureNum);

    for (uint32_t i = 0; i < resourceGroupDesc.textureNum; i++) {
        RETURN_ON_FAILURE(this, resourceGroupDesc.textures[i] != nullptr, 0, "CalculateAllocationNumber: 'resourceGroupDesc.textures[%u]' is NULL", i);

        TextureVal& textureVal = *(TextureVal*)resourceGroupDesc.textures[i];
        texturesImpl[i] = textureVal.GetImpl();
    }

    ResourceGroupDesc resourceGroupDescImpl = resourceGroupDesc;
    resourceGroupDescImpl.buffers = buffersImpl;
    resourceGroupDescImpl.textures = texturesImpl;

    return m_HelperAPI.CalculateAllocationNumber(m_Device, resourceGroupDescImpl);
}

Result DeviceVal::AllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations) {
    RETURN_ON_FAILURE(this, allocations != nullptr, Result::INVALID_ARGUMENT, "AllocateAndBindMemory: 'allocations' is NULL");

    RETURN_ON_FAILURE(
        this, resourceGroupDesc.memoryLocation < MemoryLocation::MAX_NUM, Result::INVALID_ARGUMENT, "AllocateAndBindMemory: 'resourceGroupDesc.memoryLocation' is invalid");

    RETURN_ON_FAILURE(
        this, resourceGroupDesc.bufferNum == 0 || resourceGroupDesc.buffers != nullptr, Result::INVALID_ARGUMENT, "AllocateAndBindMemory: 'resourceGroupDesc.buffers' is NULL");

    RETURN_ON_FAILURE(
        this, resourceGroupDesc.textureNum == 0 || resourceGroupDesc.textures != nullptr, Result::INVALID_ARGUMENT, "AllocateAndBindMemory: 'resourceGroupDesc.textures' is NULL");

    Buffer** buffersImpl = STACK_ALLOC(Buffer*, resourceGroupDesc.bufferNum);

    for (uint32_t i = 0; i < resourceGroupDesc.bufferNum; i++) {
        RETURN_ON_FAILURE(this, resourceGroupDesc.buffers[i] != nullptr, Result::INVALID_ARGUMENT, "AllocateAndBindMemory: 'resourceGroupDesc.buffers[%u]' is NULL", i);

        BufferVal& bufferVal = *(BufferVal*)resourceGroupDesc.buffers[i];
        buffersImpl[i] = bufferVal.GetImpl();
    }

    Texture** texturesImpl = STACK_ALLOC(Texture*, resourceGroupDesc.textureNum);

    for (uint32_t i = 0; i < resourceGroupDesc.textureNum; i++) {
        RETURN_ON_FAILURE(this, resourceGroupDesc.textures[i] != nullptr, Result::INVALID_ARGUMENT, "AllocateAndBindMemory: 'resourceGroupDesc.textures[%u]' is NULL", i);

        TextureVal& textureVal = *(TextureVal*)resourceGroupDesc.textures[i];
        texturesImpl[i] = textureVal.GetImpl();
    }

    const size_t allocationNum = CalculateAllocationNumber(resourceGroupDesc);

    ResourceGroupDesc resourceGroupDescImpl = resourceGroupDesc;
    resourceGroupDescImpl.buffers = buffersImpl;
    resourceGroupDescImpl.textures = texturesImpl;

    const Result result = m_HelperAPI.AllocateAndBindMemory(m_Device, resourceGroupDescImpl, allocations);

    if (result == Result::SUCCESS) {
        for (uint32_t i = 0; i < resourceGroupDesc.bufferNum; i++) {
            BufferVal& bufferVal = *(BufferVal*)resourceGroupDesc.buffers[i];
            bufferVal.SetBoundToMemory();
        }

        for (uint32_t i = 0; i < resourceGroupDesc.textureNum; i++) {
            TextureVal& textureVal = *(TextureVal*)resourceGroupDesc.textures[i];
            textureVal.SetBoundToMemory();
        }

        for (uint32_t i = 0; i < allocationNum; i++) {
            RETURN_ON_FAILURE(this, allocations[i] != nullptr, Result::FAILURE, "AllocateAndBindMemory: 'impl' is NULL");
            allocations[i] = (Memory*)Allocate<MemoryVal>(GetStdAllocator(), *this, allocations[i], 0, resourceGroupDesc.memoryLocation);
        }
    }

    return result;
}

Result DeviceVal::CreateRayTracingPipeline(const RayTracingPipelineDesc& pipelineDesc, Pipeline*& pipeline) {
    RETURN_ON_FAILURE(this, pipelineDesc.pipelineLayout != nullptr, Result::INVALID_ARGUMENT, "CreateRayTracingPipeline: 'pipelineDesc.pipelineLayout' is NULL");
    RETURN_ON_FAILURE(this, pipelineDesc.shaderLibrary != nullptr, Result::INVALID_ARGUMENT, "CreateRayTracingPipeline: 'pipelineDesc.shaderLibrary' is NULL");
    RETURN_ON_FAILURE(this, pipelineDesc.shaderGroupDescs != nullptr, Result::INVALID_ARGUMENT, "CreateRayTracingPipeline: 'pipelineDesc.shaderGroupDescs' is NULL");
    RETURN_ON_FAILURE(this, pipelineDesc.shaderGroupDescNum != 0, Result::INVALID_ARGUMENT, "CreateRayTracingPipeline: 'pipelineDesc.shaderGroupDescNum' is 0");
    RETURN_ON_FAILURE(this, pipelineDesc.recursionDepthMax != 0, Result::INVALID_ARGUMENT, "CreateRayTracingPipeline: 'pipelineDesc.recursionDepthMax' is 0");

    uint32_t uniqueShaderStages = 0;
    for (uint32_t i = 0; i < pipelineDesc.shaderLibrary->shaderNum; i++) {
        const ShaderDesc& shaderDesc = pipelineDesc.shaderLibrary->shaders[i];

        RETURN_ON_FAILURE(
            this, shaderDesc.bytecode != nullptr, Result::INVALID_ARGUMENT, "CreateRayTracingPipeline: 'pipelineDesc.shaderLibrary->shaders[%u].bytecode' is invalid", i);

        RETURN_ON_FAILURE(this, shaderDesc.size != 0, Result::INVALID_ARGUMENT, "CreateRayTracingPipeline: 'pipelineDesc.shaderLibrary->shaders[%u].size' is 0", i);
        RETURN_ON_FAILURE(this, IsShaderStageValid(shaderDesc.stage, uniqueShaderStages, StageBits::RAY_TRACING_SHADERS), Result::INVALID_ARGUMENT,
            "CreateRayTracingPipeline: 'pipelineDesc.shaderLibrary->shaders[%u].stage' must include only 1 ray tracing shader stage, unique for the entire pipeline", i);
    }

    auto pipelineDescImpl = pipelineDesc;
    pipelineDescImpl.pipelineLayout = NRI_GET_IMPL(PipelineLayout, pipelineDesc.pipelineLayout);

    Pipeline* pipelineImpl = nullptr;
    const Result result = m_RayTracingAPI.CreateRayTracingPipeline(m_Device, pipelineDescImpl, pipelineImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, pipelineImpl != nullptr, Result::FAILURE, "CreateRayTracingPipeline: 'impl' is NULL");
        pipeline = (Pipeline*)Allocate<PipelineVal>(GetStdAllocator(), *this, pipelineImpl);
    }

    return result;
}

Result DeviceVal::CreateAccelerationStructure(const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    RETURN_ON_FAILURE(this, accelerationStructureDesc.instanceOrGeometryObjectNum != 0, Result::INVALID_ARGUMENT,
        "CreateAccelerationStructure: 'accelerationStructureDesc.instanceOrGeometryObjectNum' is 0");

    AccelerationStructureDesc accelerationStructureDescImpl = accelerationStructureDesc;

    Vector<GeometryObject> objectImplArray(GetStdAllocator());
    if (accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL) {
        const uint32_t geometryObjectNum = accelerationStructureDesc.instanceOrGeometryObjectNum;
        objectImplArray.resize(geometryObjectNum);
        ConvertGeometryObjectsVal(objectImplArray.data(), accelerationStructureDesc.geometryObjects, geometryObjectNum);
        accelerationStructureDescImpl.geometryObjects = objectImplArray.data();
    }

    AccelerationStructure* accelerationStructureImpl = nullptr;
    const Result result = m_RayTracingAPI.CreateAccelerationStructure(m_Device, accelerationStructureDescImpl, accelerationStructureImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(this, accelerationStructureImpl != nullptr, Result::FAILURE, "CreateAccelerationStructure: 'impl' is NULL");
        accelerationStructure = (AccelerationStructure*)Allocate<AccelerationStructureVal>(GetStdAllocator(), *this, accelerationStructureImpl, false);
    }

    return result;
}

Result DeviceVal::BindAccelerationStructureMemory(const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    if (memoryBindingDescNum == 0)
        return Result::SUCCESS;

    RETURN_ON_FAILURE(this, memoryBindingDescs != nullptr, Result::INVALID_ARGUMENT, "BindAccelerationStructureMemory: 'memoryBindingDescs' is NULL");

    AccelerationStructureMemoryBindingDesc* memoryBindingDescsImpl = STACK_ALLOC(AccelerationStructureMemoryBindingDesc, memoryBindingDescNum);
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        AccelerationStructureMemoryBindingDesc& destDesc = memoryBindingDescsImpl[i];
        const AccelerationStructureMemoryBindingDesc& srcDesc = memoryBindingDescs[i];

        MemoryVal& memory = (MemoryVal&)*srcDesc.memory;
        AccelerationStructureVal& accelerationStructure = (AccelerationStructureVal&)*srcDesc.accelerationStructure;

        RETURN_ON_FAILURE(this, !accelerationStructure.IsBoundToMemory(), Result::INVALID_ARGUMENT,
            "BindAccelerationStructureMemory: 'memoryBindingDescs[%u].accelerationStructure' is already bound to memory", i);

        MemoryDesc memoryDesc = {};
        accelerationStructure.GetMemoryInfo(memoryDesc);

        RETURN_ON_FAILURE(this, !memoryDesc.mustBeDedicated || srcDesc.offset == 0, Result::INVALID_ARGUMENT,
            "BindAccelerationStructureMemory: 'memoryBindingDescs[%u].offset' must be 0 for dedicated allocation", i);

        RETURN_ON_FAILURE(this, memoryDesc.alignment != 0, Result::INVALID_ARGUMENT, "BindAccelerationStructureMemory: 'memoryBindingDescs[%u].alignment' is 0", i);

        RETURN_ON_FAILURE(
            this, srcDesc.offset % memoryDesc.alignment == 0, Result::INVALID_ARGUMENT, "BindAccelerationStructureMemory: 'memoryBindingDescs[%u].offset' is misaligned", i);

        const uint64_t rangeMax = srcDesc.offset + memoryDesc.size;
        const bool memorySizeIsUnknown = memory.GetSize() == 0;

        RETURN_ON_FAILURE(
            this, memorySizeIsUnknown || rangeMax <= memory.GetSize(), Result::INVALID_ARGUMENT, "BindAccelerationStructureMemory: 'memoryBindingDescs[%u].offset' is invalid", i);

        destDesc = srcDesc;
        destDesc.memory = memory.GetImpl();
        destDesc.accelerationStructure = accelerationStructure.GetImpl();
    }

    const Result result = m_RayTracingAPI.BindAccelerationStructureMemory(m_Device, memoryBindingDescsImpl, memoryBindingDescNum);

    if (result == Result::SUCCESS) {
        for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
            MemoryVal& memory = *(MemoryVal*)memoryBindingDescs[i].memory;
            memory.BindAccelerationStructure(*(AccelerationStructureVal*)memoryBindingDescs[i].accelerationStructure);
        }
    }

    return result;
}

void DeviceVal::DestroyAccelerationStructure(AccelerationStructure& accelerationStructure) {
    Deallocate(GetStdAllocator(), (AccelerationStructureVal*)&accelerationStructure);
}

void DeviceVal::Destroy() {
    Deallocate(GetStdAllocator(), this);
}

DeviceBase* CreateDeviceValidation(const DeviceCreationDesc& deviceCreationDesc, DeviceBase& device) {
    StdAllocator<uint8_t> allocator(deviceCreationDesc.memoryAllocatorInterface);
    DeviceVal* deviceVal = Allocate<DeviceVal>(allocator, deviceCreationDesc.callbackInterface, allocator, device);

    if (!deviceVal->Create()) {
        Deallocate(allocator, deviceVal);
        return nullptr;
    }

    return deviceVal;
}

#include "DeviceVal.hpp"
