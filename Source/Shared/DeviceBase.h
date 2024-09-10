// © 2021 NVIDIA Corporation

#pragma once

namespace nri {
struct DeviceBase {
    inline DeviceBase(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator)
        : m_CallbackInterface(callbacks)
        , m_StdAllocator(stdAllocator) {
    }

    inline StdAllocator<uint8_t>& GetStdAllocator() {
        return m_StdAllocator;
    }

    void ReportMessage(Message messageType, const char* file, uint32_t line, const char* format, ...) const;

    template <typename T>
    Result ValidateFunctionTable(const T& table) const {
        const void* const* const begin = (void**)&table;
        const void* const* const end = (void**)(&table + 1);
        for (const void* const* current = begin; current != end; current++) {
            if (*current == nullptr) {
                REPORT_ERROR(this, "Invalid function table: function #%u is NULL!", uint32_t(current - begin));
                return Result::FAILURE;
            }
        }

        return Result::SUCCESS;
    }

    virtual ~DeviceBase() {
    }

    virtual const DeviceDesc& GetDesc() const = 0;
    virtual void Destruct() = 0;

    virtual Result FillFunctionTable(CoreInterface& table) const {
        table = {};
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(HelperInterface& table) const {
        table = {};
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(LowLatencyInterface& table) const {
        table = {};
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(MeshShaderInterface& table) const {
        table = {};
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(RayTracingInterface& table) const {
        table = {};
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(StreamerInterface& table) const {
        table = {};
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(SwapChainInterface& table) const {
        table = {};
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(ResourceAllocatorInterface& table) const {
        table = {};
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(WrapperD3D11Interface& table) const {
        table = {};
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(WrapperD3D12Interface& table) const {
        table = {};
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(WrapperVKInterface& table) const {
        table = {};
        return Result::UNSUPPORTED;
    }

protected:
    CallbackInterface m_CallbackInterface = {};
    StdAllocator<uint8_t> m_StdAllocator;
};
} // namespace nri

#define Declare_PartiallyFillFunctionTable_Functions(API) \
    void Core_Buffer_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_CommandAllocator_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_CommandBuffer_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_CommandQueue_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_Descriptor_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_DescriptorPool_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_DescriptorSet_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_Device_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_Fence_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_QueryPool_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Core_Texture_PartiallyFillFunctionTable##API(CoreInterface& table); \
    void Helper_CommandQueue_PartiallyFillFunctionTable##API(HelperInterface& table); \
    void Helper_Device_PartiallyFillFunctionTable##API(HelperInterface& table); \
    void LowLatency_CommandQueue_PartiallyFillFunctionTable##API(LowLatencyInterface& table); \
    void LowLatency_SwapChain_SwapChain_PartiallyFillFunctionTable##API(LowLatencyInterface& table); \
    void MeshShader_CommandBuffer_PartiallyFillFunctionTable##API(MeshShaderInterface& table); \
    void RayTracing_AccelerationStructure_PartiallyFillFunctionTable##API(RayTracingInterface& table); \
    void RayTracing_CommandBuffer_PartiallyFillFunctionTable##API(RayTracingInterface& table); \
    void RayTracing_Device_PartiallyFillFunctionTable##API(RayTracingInterface& table); \
    void Streamer_Device_PartiallyFillFunctionTable##API(StreamerInterface& table); \
    void SwapChain_SwapChain_PartiallyFillFunctionTable##API(SwapChainInterface& table); \
    void SwapChain_Device_PartiallyFillFunctionTable##API(SwapChainInterface& table); \
    void ResourceAllocator_Device_PartiallyFillFunctionTable##API(ResourceAllocatorInterface& table); \
    void WrapperD3D11_Device_PartiallyFillFunctionTable##API(WrapperD3D11Interface& table); \
    void WrapperD3D12_Device_PartiallyFillFunctionTable##API(WrapperD3D12Interface& table); \
    void WrapperVK_Device_PartiallyFillFunctionTable##API(WrapperVKInterface& table)

#define Define_Core_Buffer_PartiallyFillFunctionTable(API) \
    void Core_Buffer_PartiallyFillFunctionTable##API(CoreInterface& table) { \
        table.SetBufferDebugName = ::SetBufferDebugName; \
        table.GetBufferNativeObject = ::GetBufferNativeObject; \
        table.MapBuffer = ::MapBuffer; \
        table.UnmapBuffer = ::UnmapBuffer; \
    }

#define Define_Core_CommandAllocator_PartiallyFillFunctionTable(API) \
    void Core_CommandAllocator_PartiallyFillFunctionTable##API(CoreInterface& table) { \
        table.SetCommandAllocatorDebugName = ::SetCommandAllocatorDebugName; \
        table.CreateCommandBuffer = ::CreateCommandBuffer; \
        table.ResetCommandAllocator = ::ResetCommandAllocator; \
    }

#define Define_Core_CommandBuffer_PartiallyFillFunctionTable(API) \
    void Core_CommandBuffer_PartiallyFillFunctionTable##API(CoreInterface& table) { \
        table.BeginCommandBuffer = ::BeginCommandBuffer; \
        table.CmdSetDescriptorPool = ::CmdSetDescriptorPool; \
        table.CmdSetDescriptorSet = ::CmdSetDescriptorSet; \
        table.CmdSetPipelineLayout = ::CmdSetPipelineLayout; \
        table.CmdSetPipeline = ::CmdSetPipeline; \
        table.CmdSetConstants = ::CmdSetConstants; \
        table.CmdBarrier = ::CmdBarrier; \
        table.CmdBeginRendering = ::CmdBeginRendering; \
        table.CmdClearAttachments = ::CmdClearAttachments; \
        table.CmdSetViewports = ::CmdSetViewports; \
        table.CmdSetScissors = ::CmdSetScissors; \
        table.CmdSetDepthBounds = ::CmdSetDepthBounds; \
        table.CmdSetStencilReference = ::CmdSetStencilReference; \
        table.CmdSetSampleLocations = ::CmdSetSampleLocations; \
        table.CmdSetBlendConstants = ::CmdSetBlendConstants; \
        table.CmdSetShadingRate = ::CmdSetShadingRate; \
        table.CmdSetDepthBias = ::CmdSetDepthBias; \
        table.CmdSetIndexBuffer = ::CmdSetIndexBuffer; \
        table.CmdSetVertexBuffers = ::CmdSetVertexBuffers; \
        table.CmdDraw = ::CmdDraw; \
        table.CmdDrawIndexed = ::CmdDrawIndexed; \
        table.CmdDrawIndirect = ::CmdDrawIndirect; \
        table.CmdDrawIndexedIndirect = ::CmdDrawIndexedIndirect; \
        table.CmdEndRendering = ::CmdEndRendering; \
        table.CmdDispatch = ::CmdDispatch; \
        table.CmdDispatchIndirect = ::CmdDispatchIndirect; \
        table.CmdBeginQuery = ::CmdBeginQuery; \
        table.CmdEndQuery = ::CmdEndQuery; \
        table.CmdCopyQueries = ::CmdCopyQueries; \
        table.CmdResetQueries = ::CmdResetQueries; \
        table.CmdBeginAnnotation = ::CmdBeginAnnotation; \
        table.CmdEndAnnotation = ::CmdEndAnnotation; \
        table.CmdClearStorageBuffer = ::CmdClearStorageBuffer; \
        table.CmdClearStorageTexture = ::CmdClearStorageTexture; \
        table.CmdCopyBuffer = ::CmdCopyBuffer; \
        table.CmdCopyTexture = ::CmdCopyTexture; \
        table.CmdUploadBufferToTexture = ::CmdUploadBufferToTexture; \
        table.CmdReadbackTextureToBuffer = ::CmdReadbackTextureToBuffer; \
        table.EndCommandBuffer = ::EndCommandBuffer; \
        table.SetCommandBufferDebugName = ::SetCommandBufferDebugName; \
        table.GetCommandBufferNativeObject = ::GetCommandBufferNativeObject; \
    }

#define Define_Core_CommandQueue_PartiallyFillFunctionTable(API) \
    void Core_CommandQueue_PartiallyFillFunctionTable##API(CoreInterface& table) { \
        table.SetCommandQueueDebugName = ::SetCommandQueueDebugName; \
        table.QueueSubmit = ::QueueSubmit; \
    }

#define Define_Core_Descriptor_PartiallyFillFunctionTable(API) \
    void Core_Descriptor_PartiallyFillFunctionTable##API(CoreInterface& table) { \
        table.SetDescriptorDebugName = ::SetDescriptorDebugName; \
        table.GetDescriptorNativeObject = ::GetDescriptorNativeObject; \
    }

#define Define_Core_DescriptorPool_PartiallyFillFunctionTable(API) \
    void Core_DescriptorPool_PartiallyFillFunctionTable##API(CoreInterface& table) { \
        table.SetDescriptorPoolDebugName = ::SetDescriptorPoolDebugName; \
        table.AllocateDescriptorSets = ::AllocateDescriptorSets; \
        table.ResetDescriptorPool = ::ResetDescriptorPool; \
    }

#define Define_Core_DescriptorSet_PartiallyFillFunctionTable(API) \
    void Core_DescriptorSet_PartiallyFillFunctionTable##API(CoreInterface& table) { \
        table.SetDescriptorSetDebugName = ::SetDescriptorSetDebugName; \
        table.UpdateDescriptorRanges = ::UpdateDescriptorRanges; \
        table.UpdateDynamicConstantBuffers = ::UpdateDynamicConstantBuffers; \
        table.CopyDescriptorSet = ::CopyDescriptorSet; \
    }

#define Define_Core_Device_PartiallyFillFunctionTable(API) \
    void Core_Device_PartiallyFillFunctionTable##API(CoreInterface& table) { \
        table.GetDeviceDesc = ::GetDeviceDesc; \
        table.GetBufferDesc = ::GetBufferDesc; \
        table.GetTextureDesc = ::GetTextureDesc; \
        table.GetFormatSupport = ::GetFormatSupport; \
        table.GetBufferMemoryDesc = ::GetBufferMemoryDesc; \
        table.GetTextureMemoryDesc = ::GetTextureMemoryDesc; \
        table.GetCommandQueue = ::GetCommandQueue; \
        table.CreateCommandAllocator = ::CreateCommandAllocator; \
        table.CreateDescriptorPool = ::CreateDescriptorPool; \
        table.CreateBuffer = ::CreateBuffer; \
        table.CreateTexture = ::CreateTexture; \
        table.CreateBufferView = ::CreateBufferView; \
        table.CreateTexture1DView = ::CreateTexture1DView; \
        table.CreateTexture2DView = ::CreateTexture2DView; \
        table.CreateTexture3DView = ::CreateTexture3DView; \
        table.CreateSampler = ::CreateSampler; \
        table.CreatePipelineLayout = ::CreatePipelineLayout; \
        table.CreateGraphicsPipeline = ::CreateGraphicsPipeline; \
        table.CreateComputePipeline = ::CreateComputePipeline; \
        table.CreateQueryPool = ::CreateQueryPool; \
        table.CreateFence = ::CreateFence; \
        table.DestroyCommandBuffer = ::DestroyCommandBuffer; \
        table.DestroyCommandAllocator = ::DestroyCommandAllocator; \
        table.DestroyDescriptorPool = ::DestroyDescriptorPool; \
        table.DestroyBuffer = ::DestroyBuffer; \
        table.DestroyTexture = ::DestroyTexture; \
        table.DestroyDescriptor = ::DestroyDescriptor; \
        table.DestroyPipelineLayout = ::DestroyPipelineLayout; \
        table.DestroyPipeline = ::DestroyPipeline; \
        table.DestroyQueryPool = ::DestroyQueryPool; \
        table.DestroyFence = ::DestroyFence; \
        table.AllocateMemory = ::AllocateMemory; \
        table.BindBufferMemory = ::BindBufferMemory; \
        table.BindTextureMemory = ::BindTextureMemory; \
        table.FreeMemory = ::FreeMemory; \
        table.SetDeviceDebugName = ::SetDeviceDebugName; \
        table.SetPipelineDebugName = ::SetPipelineDebugName; \
        table.SetPipelineLayoutDebugName = ::SetPipelineLayoutDebugName; \
        table.SetMemoryDebugName = ::SetMemoryDebugName; \
        table.GetDeviceNativeObject = ::GetDeviceNativeObject; \
    }

#define Define_Core_Fence_PartiallyFillFunctionTable(API) \
    void Core_Fence_PartiallyFillFunctionTable##API(CoreInterface& table) { \
        table.GetFenceValue = ::GetFenceValue; \
        table.Wait = ::Wait; \
        table.SetFenceDebugName = ::SetFenceDebugName; \
    }

#define Define_Core_QueryPool_PartiallyFillFunctionTable(API) \
    void Core_QueryPool_PartiallyFillFunctionTable##API(CoreInterface& table) { \
        table.SetQueryPoolDebugName = ::SetQueryPoolDebugName; \
        table.GetQuerySize = ::GetQuerySize; \
    }

#define Define_Core_Texture_PartiallyFillFunctionTable(API) \
    void Core_Texture_PartiallyFillFunctionTable##API(CoreInterface& table) { \
        table.SetTextureDebugName = ::SetTextureDebugName; \
        table.GetTextureNativeObject = ::GetTextureNativeObject; \
    }

#define Define_Helper_CommandQueue_PartiallyFillFunctionTable(API) \
    void Helper_CommandQueue_PartiallyFillFunctionTable##API(HelperInterface& table) { \
        table.UploadData = ::UploadData; \
        table.WaitForIdle = ::WaitForIdle; \
    }

#define Define_Helper_Device_PartiallyFillFunctionTable(API) \
    void Helper_Device_PartiallyFillFunctionTable##API(HelperInterface& table) { \
        table.CalculateAllocationNumber = ::CalculateAllocationNumber; \
        table.AllocateAndBindMemory = ::AllocateAndBindMemory; \
        table.QueryVideoMemoryInfo = ::QueryVideoMemoryInfo; \
    }

#define Define_LowLatency_CommandQueue_PartiallyFillFunctionTable(API) \
    void LowLatency_CommandQueue_PartiallyFillFunctionTable##API(LowLatencyInterface& table) { \
        table.QueueSubmitTrackable = ::QueueSubmitTrackable; \
    }

#define Define_LowLatency_SwapChain_SwapChain_PartiallyFillFunctionTable(API) \
    void LowLatency_SwapChain_SwapChain_PartiallyFillFunctionTable##API(LowLatencyInterface& table) { \
        table.SetLatencySleepMode = ::SetLatencySleepMode; \
        table.SetLatencyMarker = ::SetLatencyMarker; \
        table.LatencySleep = ::LatencySleep; \
        table.GetLatencyReport = ::GetLatencyReport; \
    }

#define Define_MeshShader_CommandBuffer_PartiallyFillFunctionTable(API) \
    void MeshShader_CommandBuffer_PartiallyFillFunctionTable##API(MeshShaderInterface& table) { \
        table.CmdDrawMeshTasks = ::CmdDrawMeshTasks; \
        table.CmdDrawMeshTasksIndirect = ::CmdDrawMeshTasksIndirect; \
    }

#define Define_RayTracing_AccelerationStructure_PartiallyFillFunctionTable(API) \
    void RayTracing_AccelerationStructure_PartiallyFillFunctionTable##API(RayTracingInterface& table) { \
        table.CreateAccelerationStructureDescriptor = ::CreateAccelerationStructureDescriptor; \
        table.GetAccelerationStructureUpdateScratchBufferSize = ::GetAccelerationStructureUpdateScratchBufferSize; \
        table.GetAccelerationStructureBuildScratchBufferSize = ::GetAccelerationStructureBuildScratchBufferSize; \
        table.GetAccelerationStructureHandle = ::GetAccelerationStructureHandle; \
        table.SetAccelerationStructureDebugName = ::SetAccelerationStructureDebugName; \
        table.GetAccelerationStructureNativeObject = ::GetAccelerationStructureNativeObject; \
    }

#define Define_RayTracing_CommandBuffer_PartiallyFillFunctionTable(API) \
    void RayTracing_CommandBuffer_PartiallyFillFunctionTable##API(RayTracingInterface& table) { \
        table.CmdBuildTopLevelAccelerationStructure = ::CmdBuildTopLevelAccelerationStructure; \
        table.CmdBuildBottomLevelAccelerationStructure = ::CmdBuildBottomLevelAccelerationStructure; \
        table.CmdUpdateTopLevelAccelerationStructure = ::CmdUpdateTopLevelAccelerationStructure; \
        table.CmdUpdateBottomLevelAccelerationStructure = ::CmdUpdateBottomLevelAccelerationStructure; \
        table.CmdCopyAccelerationStructure = ::CmdCopyAccelerationStructure; \
        table.CmdWriteAccelerationStructureSize = ::CmdWriteAccelerationStructureSize; \
        table.CmdDispatchRays = ::CmdDispatchRays; \
        table.CmdDispatchRaysIndirect = ::CmdDispatchRaysIndirect; \
    }

#define Define_RayTracing_Device_PartiallyFillFunctionTable(API) \
    void RayTracing_Device_PartiallyFillFunctionTable##API(RayTracingInterface& table) { \
        table.GetAccelerationStructureMemoryDesc = ::GetAccelerationStructureMemoryDesc; \
        table.CreateRayTracingPipeline = ::CreateRayTracingPipeline; \
        table.CreateAccelerationStructure = ::CreateAccelerationStructure; \
        table.BindAccelerationStructureMemory = ::BindAccelerationStructureMemory; \
        table.DestroyAccelerationStructure = ::DestroyAccelerationStructure; \
    }

#define Define_Streamer_Device_PartiallyFillFunctionTable(API) \
    void Streamer_Device_PartiallyFillFunctionTable##API(StreamerInterface& table) { \
        table.CreateStreamer = ::CreateStreamer; \
        table.DestroyStreamer = ::DestroyStreamer; \
        table.GetStreamerConstantBuffer = ::GetStreamerConstantBuffer; \
        table.UpdateStreamerConstantBuffer = ::UpdateStreamerConstantBuffer; \
        table.AddStreamerBufferUpdateRequest = ::AddStreamerBufferUpdateRequest; \
        table.AddStreamerTextureUpdateRequest = ::AddStreamerTextureUpdateRequest; \
        table.CopyStreamerUpdateRequests = ::CopyStreamerUpdateRequests; \
        table.GetStreamerDynamicBuffer = ::GetStreamerDynamicBuffer; \
        table.CmdUploadStreamerUpdateRequests = ::CmdUploadStreamerUpdateRequests; \
    }

#define Define_SwapChain_Device_PartiallyFillFunctionTable(API) \
    void SwapChain_Device_PartiallyFillFunctionTable##API(SwapChainInterface& table) { \
        table.CreateSwapChain = ::CreateSwapChain; \
        table.DestroySwapChain = ::DestroySwapChain; \
    }

#define Define_SwapChain_SwapChain_PartiallyFillFunctionTable(API) \
    void SwapChain_SwapChain_PartiallyFillFunctionTable##API(SwapChainInterface& table) { \
        table.SetSwapChainDebugName = ::SetSwapChainDebugName; \
        table.GetSwapChainTextures = ::GetSwapChainTextures; \
        table.AcquireNextSwapChainTexture = ::AcquireNextSwapChainTexture; \
        table.WaitForPresent = ::WaitForPresent; \
        table.QueuePresent = ::QueuePresent; \
        table.GetDisplayDesc = ::GetDisplayDesc; \
    }

#define Define_ResourceAllocator_Device_PartiallyFillFunctionTable(API) \
    void ResourceAllocator_Device_PartiallyFillFunctionTable##API(ResourceAllocatorInterface& table) { \
        table.AllocateBuffer = ::AllocateBuffer; \
        table.AllocateTexture = ::AllocateTexture; \
        table.AllocateAccelerationStructure = ::AllocateAccelerationStructure; \
    }

#define Define_WrapperD3D11_Device_PartiallyFillFunctionTable(API) \
    void WrapperD3D11_Device_PartiallyFillFunctionTable##API(WrapperD3D11Interface& table) { \
        table.CreateCommandBufferD3D11 = ::CreateCommandBufferD3D11; \
        table.CreateTextureD3D11 = ::CreateTextureD3D11; \
        table.CreateBufferD3D11 = ::CreateBufferD3D11; \
    }

#define Define_WrapperD3D12_Device_PartiallyFillFunctionTable(API) \
    void WrapperD3D12_Device_PartiallyFillFunctionTable##API(WrapperD3D12Interface& table) { \
        table.CreateCommandBufferD3D12 = ::CreateCommandBufferD3D12; \
        table.CreateDescriptorPoolD3D12 = ::CreateDescriptorPoolD3D12; \
        table.CreateBufferD3D12 = ::CreateBufferD3D12; \
        table.CreateTextureD3D12 = ::CreateTextureD3D12; \
        table.CreateMemoryD3D12 = ::CreateMemoryD3D12; \
        table.CreateAccelerationStructureD3D12 = ::CreateAccelerationStructureD3D12; \
    }

#define Define_WrapperVK_Device_PartiallyFillFunctionTable(API) \
    void WrapperVK_Device_PartiallyFillFunctionTable##API(WrapperVKInterface& table) { \
        table.CreateCommandQueueVK = ::CreateCommandQueueVK; \
        table.CreateCommandAllocatorVK = ::CreateCommandAllocatorVK; \
        table.CreateCommandBufferVK = ::CreateCommandBufferVK; \
        table.CreateDescriptorPoolVK = ::CreateDescriptorPoolVK; \
        table.CreateBufferVK = ::CreateBufferVK; \
        table.CreateTextureVK = ::CreateTextureVK; \
        table.CreateMemoryVK = ::CreateMemoryVK; \
        table.CreateGraphicsPipelineVK = ::CreateGraphicsPipelineVK; \
        table.CreateComputePipelineVK = ::CreateComputePipelineVK; \
        table.CreateQueryPoolVK = ::CreateQueryPoolVK; \
        table.CreateAccelerationStructureVK = ::CreateAccelerationStructureVK; \
        table.GetPhysicalDeviceVK = ::GetPhysicalDeviceVK; \
        table.GetInstanceVK = ::GetInstanceVK; \
        table.GetDeviceProcAddrVK = ::GetDeviceProcAddrVK; \
        table.GetInstanceProcAddrVK = ::GetInstanceProcAddrVK; \
    }
