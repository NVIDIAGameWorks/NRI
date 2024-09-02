// © 2021 NVIDIA Corporation

/*
Overview:
- Generalized common denominator for VK, D3D12 and D3D11
    - VK spec: https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html
    - D3D12 spec: https://microsoft.github.io/DirectX-Specs/
    - D3D11 spec: https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm
- C/C++ compatible interface (auto-selection or via "NRI_FORCE_C" macro)

Goals:
- generalization of VK and D3D12
- providing access to low-level features of modern gAPIs
- low overhead
- D3D11 support (as much as possible)

Non-goals:
- high level RHI
- exposing entities not existing in gAPIs
- D3D11-like abstraction level
- hidden management of any kind
*/

#pragma once

#define NRI_VERSION_MAJOR 1
#define NRI_VERSION_MINOR 146
#define NRI_VERSION_DATE "2 September 2024"

#include "NRIDescs.h"

NriNamespaceBegin

// Example: Result result = nriGetInterface(device, NRI_INTERFACE(CoreInterface), &coreInterface)
NRI_API Nri(Result) NRI_CALL nriGetInterface(const NriRef(Device) device, const char* interfaceName, size_t interfaceSize, void* interfacePtr);

NriStruct(CoreInterface) {
    // Get
    const NriRef(DeviceDesc) (NRI_CALL *GetDeviceDesc)(const NriRef(Device) device);
    const NriRef(BufferDesc) (NRI_CALL *GetBufferDesc)(const NriRef(Buffer) buffer);
    const NriRef(TextureDesc) (NRI_CALL *GetTextureDesc)(const NriRef(Texture) texture);
    Nri(FormatSupportBits) (NRI_CALL *GetFormatSupport)(const NriRef(Device) device, Nri(Format) format);
    uint32_t (NRI_CALL *GetQuerySize)(const NriRef(QueryPool) queryPool);
    void (NRI_CALL *GetBufferMemoryDesc)(const NriRef(Device) device, const NriRef(BufferDesc) bufferDesc, Nri(MemoryLocation) memoryLocation, NriOut NriRef(MemoryDesc) memoryDesc);
    void (NRI_CALL *GetTextureMemoryDesc)(const NriRef(Device) device, const NriRef(TextureDesc) textureDesc, Nri(MemoryLocation) memoryLocation, NriOut NriRef(MemoryDesc) memoryDesc);

    // Getting COMPUTE and/or COPY queues switches VK "sharing mode" to "VK_SHARING_MODE_CONCURRENT", which can be slower on some HW. This approach is used to avoid
    // dealing with "queue ownership transitions", but also adds a requirement to "get" all async queues before resources creation participating into multi-queue activity
    Nri(Result) (NRI_CALL *GetCommandQueue)(NriRef(Device) device, Nri(CommandQueueType) commandQueueType, NriOut NriRef(CommandQueue*) commandQueue);

    // Create
    // "Creation" doesn't assume allocation of big chunks of memory on the device, but it happens for some entities implicitly
    // "Allocation" emphasizes the fact that there is a chunk of memory allocated under the hood
    Nri(Result) (NRI_CALL *CreateCommandAllocator)(const NriRef(CommandQueue) commandQueue, NriOut NriRef(CommandAllocator*) commandAllocator);
    Nri(Result) (NRI_CALL *CreateCommandBuffer)(NriRef(CommandAllocator) commandAllocator, NriOut NriRef(CommandBuffer*) commandBuffer);
    Nri(Result) (NRI_CALL *CreateDescriptorPool)(NriRef(Device) device, const NriRef(DescriptorPoolDesc) descriptorPoolDesc, NriOut NriRef(DescriptorPool*) descriptorPool);
    Nri(Result) (NRI_CALL *CreateBuffer)(NriRef(Device) device, const NriRef(BufferDesc) bufferDesc, NriOut NriRef(Buffer*) buffer); // requires "BindBufferMemory"
    Nri(Result) (NRI_CALL *CreateTexture)(NriRef(Device) device, const NriRef(TextureDesc) textureDesc, NriOut NriRef(Texture*) texture); // requires "BindTextureMemory"
    Nri(Result) (NRI_CALL *CreateBufferView)(const NriRef(BufferViewDesc) bufferViewDesc, NriOut NriRef(Descriptor*) bufferView);
    Nri(Result) (NRI_CALL *CreateTexture1DView)(const NriRef(Texture1DViewDesc) textureViewDesc, NriOut NriRef(Descriptor*) textureView);
    Nri(Result) (NRI_CALL *CreateTexture2DView)(const NriRef(Texture2DViewDesc) textureViewDesc, NriOut NriRef(Descriptor*) textureView);
    Nri(Result) (NRI_CALL *CreateTexture3DView)(const NriRef(Texture3DViewDesc) textureViewDesc, NriOut NriRef(Descriptor*) textureView);
    Nri(Result) (NRI_CALL *CreateSampler)(NriRef(Device) device, const NriRef(SamplerDesc) samplerDesc, NriOut NriRef(Descriptor*) sampler);
    Nri(Result) (NRI_CALL *CreatePipelineLayout)(NriRef(Device) device, const NriRef(PipelineLayoutDesc) pipelineLayoutDesc, NriOut NriRef(PipelineLayout*) pipelineLayout);
    Nri(Result) (NRI_CALL *CreateGraphicsPipeline)(NriRef(Device) device, const NriRef(GraphicsPipelineDesc) graphicsPipelineDesc, NriOut NriRef(Pipeline*) pipeline);
    Nri(Result) (NRI_CALL *CreateComputePipeline)(NriRef(Device) device, const NriRef(ComputePipelineDesc) computePipelineDesc, NriOut NriRef(Pipeline*) pipeline);
    Nri(Result) (NRI_CALL *CreateQueryPool)(NriRef(Device) device, const NriRef(QueryPoolDesc) queryPoolDesc, NriOut NriRef(QueryPool*) queryPool);
    Nri(Result) (NRI_CALL *CreateFence)(NriRef(Device) device, uint64_t initialValue, NriOut NriRef(Fence*) fence);

    // Destroy
    void (NRI_CALL *DestroyCommandAllocator)(NriRef(CommandAllocator) commandAllocator);
    void (NRI_CALL *DestroyCommandBuffer)(NriRef(CommandBuffer) commandBuffer);
    void (NRI_CALL *DestroyDescriptorPool)(NriRef(DescriptorPool) descriptorPool);
    void (NRI_CALL *DestroyBuffer)(NriRef(Buffer) buffer);
    void (NRI_CALL *DestroyTexture)(NriRef(Texture) texture);
    void (NRI_CALL *DestroyDescriptor)(NriRef(Descriptor) descriptor);
    void (NRI_CALL *DestroyPipelineLayout)(NriRef(PipelineLayout) pipelineLayout);
    void (NRI_CALL *DestroyPipeline)(NriRef(Pipeline) pipeline);
    void (NRI_CALL *DestroyQueryPool)(NriRef(QueryPool) queryPool);
    void (NRI_CALL *DestroyFence)(NriRef(Fence) fence);

    // Memory
    //  Low level:
    //      - use "Get[Resource]MemoryDesc" to get "MemoryDesc" ("usageBits" and "MemoryLocation" affect returned "MemoryType")
    //      - (optional) group returned "MemoryDesc"s by "MemoryType", but don't group if "mustBeDedicated = true"
    //      - call "Bind[Resource]Memory" to bind resources to "Memory" objects
    //  Mid level:
    //      - "CalculateAllocationNumber" and "AllocateAndBindMemory" simplify this process for buffers and textures
    //  High level:
    //      - "ResourceAllocatorInterface" allows to create resources already bound to memory
    Nri(Result) (NRI_CALL *AllocateMemory)(NriRef(Device) device, const NriRef(AllocateMemoryDesc) allocateMemoryDesc, NriOut NriRef(Memory*) memory);
    Nri(Result) (NRI_CALL *BindBufferMemory)(NriRef(Device) device, const NriPtr(BufferMemoryBindingDesc) memoryBindingDescs, uint32_t memoryBindingDescNum);
    Nri(Result) (NRI_CALL *BindTextureMemory)(NriRef(Device) device, const NriPtr(TextureMemoryBindingDesc) memoryBindingDescs, uint32_t memoryBindingDescNum);
    void (NRI_CALL *FreeMemory)(NriRef(Memory) memory);

    // Command buffer
    Nri(Result) (NRI_CALL *BeginCommandBuffer)(NriRef(CommandBuffer) commandBuffer, const NriPtr(DescriptorPool) descriptorPool);
    // {
        // Setup
        void (NRI_CALL *CmdSetDescriptorPool)(NriRef(CommandBuffer) commandBuffer, const NriRef(DescriptorPool) descriptorPool);
        void (NRI_CALL *CmdSetDescriptorSet)(NriRef(CommandBuffer) commandBuffer, uint32_t setIndexInPipelineLayout, const NriRef(DescriptorSet) descriptorSet, const uint32_t* dynamicConstantBufferOffsets);
        void (NRI_CALL *CmdSetPipelineLayout)(NriRef(CommandBuffer) commandBuffer, const NriRef(PipelineLayout) pipelineLayout);
        void (NRI_CALL *CmdSetPipeline)(NriRef(CommandBuffer) commandBuffer, const NriRef(Pipeline) pipeline);
        void (NRI_CALL *CmdSetConstants)(NriRef(CommandBuffer) commandBuffer, uint32_t pushConstantIndex, const void* data, uint32_t size);

        // Barrier
        void (NRI_CALL *CmdBarrier)(NriRef(CommandBuffer) commandBuffer, const NriRef(BarrierGroupDesc) barrierGroupDesc);

        // Input assembly
        void (NRI_CALL *CmdSetIndexBuffer)(NriRef(CommandBuffer) commandBuffer, const NriRef(Buffer) buffer, uint64_t offset, Nri(IndexType) indexType);
        void (NRI_CALL *CmdSetVertexBuffers)(NriRef(CommandBuffer) commandBuffer, uint32_t baseSlot, uint32_t bufferNum, const NriPtr(Buffer) const* buffers, const uint64_t* offsets);

        // Initial state
        void (NRI_CALL *CmdSetViewports)(NriRef(CommandBuffer) commandBuffer, const NriPtr(Viewport) viewports, uint32_t viewportNum);
        void (NRI_CALL *CmdSetScissors)(NriRef(CommandBuffer) commandBuffer, const NriPtr(Rect) rects, uint32_t rectNum);

        // Initial state, if enabled in the pipeline (since this state is global inside a command buffer in D3D11/D3D12 better treat it as global even in VK to avoid discrepancies)
        void (NRI_CALL *CmdSetStencilReference)(NriRef(CommandBuffer) commandBuffer, uint8_t frontRef, uint8_t backRef); // "backRef" requires "isIndependentFrontAndBackStencilReferenceAndMasksSupported"
        void (NRI_CALL *CmdSetDepthBounds)(NriRef(CommandBuffer) commandBuffer, float boundsMin, float boundsMax);
        void (NRI_CALL *CmdSetBlendConstants)(NriRef(CommandBuffer) commandBuffer, const NriRef(Color32f) color);
        void (NRI_CALL *CmdSetSamplePositions)(NriRef(CommandBuffer) commandBuffer, const NriPtr(SamplePosition) positions, Nri(Sample_t) positionNum, Nri(Sample_t) sampleNum); // requires "isProgrammableSampleLocationsSupported"
        void (NRI_CALL *CmdSetShadingRate)(NriRef(CommandBuffer) commandBuffer, const NriRef(ShadingRateDesc) shadingRateDesc); // requires "isShadingRateSupported"

        // State override, if enabled in the pipeline
        void (NRI_CALL *CmdSetDepthBias)(NriRef(CommandBuffer) commandBuffer, const NriRef(DepthBiasDesc) depthBiasDesc); // requires "isDynamicDepthBiasSupported"

        // Graphics
        void (NRI_CALL *CmdBeginRendering)(NriRef(CommandBuffer) commandBuffer, const NriRef(AttachmentsDesc) attachmentsDesc);
        // {
            // Fast clear
            void (NRI_CALL *CmdClearAttachments)(NriRef(CommandBuffer) commandBuffer, const NriPtr(ClearDesc) clearDescs, uint32_t clearDescNum, const NriPtr(Rect) rects, uint32_t rectNum);

            // Draw
            void (NRI_CALL *CmdDraw)(NriRef(CommandBuffer) commandBuffer, const NriRef(DrawDesc) drawDesc);
            void (NRI_CALL *CmdDrawIndexed)(NriRef(CommandBuffer) commandBuffer, const NriRef(DrawIndexedDesc) drawIndexedDesc);

            // Draw indirect:
            //  - drawNum = min(drawNum, countBuffer ? countBuffer[countBufferOffset] : INF)
            //  - "CmdDrawIndirect": "buffer" contains "Draw(Base)Desc" commands
            //  - "CmdDrawIndexedIndirect": "buffer" contains "DrawIndexed(Base)Desc" commands
            //  - see "Modified draw command signatures"
            void (NRI_CALL *CmdDrawIndirect)(NriRef(CommandBuffer) commandBuffer, const NriRef(Buffer) buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const NriPtr(Buffer) countBuffer, uint64_t countBufferOffset);
            void (NRI_CALL *CmdDrawIndexedIndirect)(NriRef(CommandBuffer) commandBuffer, const NriRef(Buffer) buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const NriPtr(Buffer) countBuffer, uint64_t countBufferOffset);
        // }
        void (NRI_CALL *CmdEndRendering)(NriRef(CommandBuffer) commandBuffer);

        // Compute
        void (NRI_CALL *CmdDispatch)(NriRef(CommandBuffer) commandBuffer, const NriRef(DispatchDesc) dispatchDesc);
        void (NRI_CALL *CmdDispatchIndirect)(NriRef(CommandBuffer) commandBuffer, const NriRef(Buffer) buffer, uint64_t offset); // buffer contains "DispatchDesc" commands

        // Copy
        void (NRI_CALL *CmdCopyBuffer)(NriRef(CommandBuffer) commandBuffer, NriRef(Buffer) dstBuffer, uint64_t dstOffset, const NriRef(Buffer) srcBuffer, uint64_t srcOffset, uint64_t size);
        void (NRI_CALL *CmdCopyTexture)(NriRef(CommandBuffer) commandBuffer, NriRef(Texture) dstTexture, const NriPtr(TextureRegionDesc) dstRegionDesc, const NriRef(Texture) srcTexture, const NriPtr(TextureRegionDesc) srcRegionDesc);
        void (NRI_CALL *CmdUploadBufferToTexture)(NriRef(CommandBuffer) commandBuffer, NriRef(Texture) dstTexture, const NriRef(TextureRegionDesc) dstRegionDesc, const NriRef(Buffer) srcBuffer, const NriRef(TextureDataLayoutDesc) srcDataLayoutDesc);
        void (NRI_CALL *CmdReadbackTextureToBuffer)(NriRef(CommandBuffer) commandBuffer, NriRef(Buffer) dstBuffer, const NriRef(TextureDataLayoutDesc) dstDataLayoutDesc, const NriRef(Texture) srcTexture, const NriRef(TextureRegionDesc) srcRegionDesc);

        // Clear storage (potentially slow clear)
        void (NRI_CALL *CmdClearStorageBuffer)(NriRef(CommandBuffer) commandBuffer, const NriRef(ClearStorageBufferDesc) clearDesc);
        void (NRI_CALL *CmdClearStorageTexture)(NriRef(CommandBuffer) commandBuffer, const NriRef(ClearStorageTextureDesc) clearDesc);

        // Query
        void (NRI_CALL *CmdResetQueries)(NriRef(CommandBuffer) commandBuffer, const NriRef(QueryPool) queryPool, uint32_t offset, uint32_t num);
        void (NRI_CALL *CmdBeginQuery)(NriRef(CommandBuffer) commandBuffer, const NriRef(QueryPool) queryPool, uint32_t offset);
        void (NRI_CALL *CmdEndQuery)(NriRef(CommandBuffer) commandBuffer, const NriRef(QueryPool) queryPool, uint32_t offset);
        void (NRI_CALL *CmdCopyQueries)(NriRef(CommandBuffer) commandBuffer, const NriRef(QueryPool) queryPool, uint32_t offset, uint32_t num, NriRef(Buffer) dstBuffer, uint64_t dstOffset);

        // Annotation
        void (NRI_CALL *CmdBeginAnnotation)(NriRef(CommandBuffer) commandBuffer, const char* name);
        void (NRI_CALL *CmdEndAnnotation)(NriRef(CommandBuffer) commandBuffer);
    // }
    Nri(Result) (NRI_CALL *EndCommandBuffer)(NriRef(CommandBuffer) commandBuffer);

    // Work submission and synchronization
    void (NRI_CALL *QueueSubmit)(NriRef(CommandQueue) commandQueue, const NriRef(QueueSubmitDesc) queueSubmitDesc); // on device
    void (NRI_CALL *Wait)(NriRef(Fence) fence, uint64_t value); // on host
    uint64_t (NRI_CALL *GetFenceValue)(NriRef(Fence) fence);

    // Descriptor set
    void (NRI_CALL *UpdateDescriptorRanges)(NriRef(DescriptorSet) descriptorSet, uint32_t baseRange, uint32_t rangeNum, const NriPtr(DescriptorRangeUpdateDesc) rangeUpdateDescs);
    void (NRI_CALL *UpdateDynamicConstantBuffers)(NriRef(DescriptorSet) descriptorSet, uint32_t baseBuffer, uint32_t bufferNum, const NriPtr(Descriptor) const* descriptors);
    void (NRI_CALL *CopyDescriptorSet)(NriRef(DescriptorSet) descriptorSet, const NriRef(DescriptorSetCopyDesc) descriptorSetCopyDesc);

    // Descriptor pool
    Nri(Result) (NRI_CALL *AllocateDescriptorSets)(NriRef(DescriptorPool) descriptorPool, const NriRef(PipelineLayout) pipelineLayout, uint32_t setIndexInPipelineLayout, NriPtr(DescriptorSet)* descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum);
    void (NRI_CALL *ResetDescriptorPool)(NriRef(DescriptorPool) descriptorPool);

    // Command allocator
    void (NRI_CALL *ResetCommandAllocator)(NriRef(CommandAllocator) commandAllocator);

    // Map / Unmap
    void* (NRI_CALL *MapBuffer)(NriRef(Buffer) buffer, uint64_t offset, uint64_t size);
    void (NRI_CALL *UnmapBuffer)(NriRef(Buffer) buffer);

    // Debug name
    void (NRI_CALL *SetDeviceDebugName)(NriRef(Device) device, const char* name);
    void (NRI_CALL *SetFenceDebugName)(NriRef(Fence) fence, const char* name);
    void (NRI_CALL *SetDescriptorDebugName)(NriRef(Descriptor) descriptor, const char* name);
    void (NRI_CALL *SetPipelineDebugName)(NriRef(Pipeline) pipeline, const char* name);
    void (NRI_CALL *SetCommandBufferDebugName)(NriRef(CommandBuffer) commandBuffer, const char* name);

    // Debug name - D3D11/D3D12: skipped if called *before* "Bind[X]Memory"
    void (NRI_CALL *SetBufferDebugName)(NriRef(Buffer) buffer, const char* name);
    void (NRI_CALL *SetTextureDebugName)(NriRef(Texture) texture, const char* name);

    // Debug name - D3D11: NOP
    void (NRI_CALL *SetCommandQueueDebugName)(NriRef(CommandQueue) commandQueue, const char* name);
    void (NRI_CALL *SetCommandAllocatorDebugName)(NriRef(CommandAllocator) commandAllocator, const char* name);
    void (NRI_CALL *SetDescriptorPoolDebugName)(NriRef(DescriptorPool) descriptorPool, const char* name);
    void (NRI_CALL *SetPipelineLayoutDebugName)(NriRef(PipelineLayout) pipelineLayout, const char* name);
    void (NRI_CALL *SetQueryPoolDebugName)(NriRef(QueryPool) queryPool, const char* name);
    void (NRI_CALL *SetDescriptorSetDebugName)(NriRef(DescriptorSet) descriptorSet, const char* name);
    void (NRI_CALL *SetMemoryDebugName)(NriRef(Memory) memory, const char* name);

    // Native objects                                                                          ___D3D11________________|_D3D12_______________________|_VK______________________
    void* (NRI_CALL *GetDeviceNativeObject)(const NriRef(Device) device);                      // ID3D11Device*        | ID3D12Device*               | VkDevice
    void* (NRI_CALL *GetCommandBufferNativeObject)(const NriRef(CommandBuffer) commandBuffer); // ID3D11DeviceContext* | ID3D12GraphicsCommandList*  | VkCommandBuffer
    uint64_t (NRI_CALL *GetBufferNativeObject)(const NriRef(Buffer) buffer);                   // ID3D11Buffer*        | ID3D12Resource*             | VkBuffer
    uint64_t (NRI_CALL *GetTextureNativeObject)(const NriRef(Texture) texture);                // ID3D11Resource*      | ID3D12Resource*             | VkImage
    uint64_t (NRI_CALL *GetDescriptorNativeObject)(const NriRef(Descriptor) descriptor);       // ID3D11View*          | D3D12_CPU_DESCRIPTOR_HANDLE | VkImageView/VkBufferView
};

// A friendly way to get a supported depth format
static inline Nri(Format) NriFunc(GetSupportedDepthFormat)(const NriRef(CoreInterface) coreInterface, const NriRef(Device) device, uint32_t minBits, bool stencil) {
    if (minBits <= 16 && !stencil) {
        if (NriDeref(coreInterface)->GetFormatSupport(device, NriScopedMember(Format, D16_UNORM)) & NriScopedMember(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
            return NriScopedMember(Format, D16_UNORM);
    }

    if (minBits <= 24) {
        if (NriDeref(coreInterface)->GetFormatSupport(device, NriScopedMember(Format, D24_UNORM_S8_UINT)) & NriScopedMember(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
            return NriScopedMember(Format, D24_UNORM_S8_UINT);
    }

    if (minBits <= 32 && !stencil) {
        if (NriDeref(coreInterface)->GetFormatSupport(device, NriScopedMember(Format, D32_SFLOAT)) & NriScopedMember(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
            return NriScopedMember(Format, D32_SFLOAT);
    }

    if (NriDeref(coreInterface)->GetFormatSupport(device, NriScopedMember(Format, D32_SFLOAT_S8_UINT_X24)) & NriScopedMember(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
        return NriScopedMember(Format, D32_SFLOAT_S8_UINT_X24);

    // Should be unreachable
    return NriScopedMember(Format, UNKNOWN);
}

NriNamespaceEnd
