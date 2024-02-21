// © 2021 NVIDIA Corporation

#include "SharedD3D12.h"

#include "AccelerationStructureD3D12.h"
#include "BufferD3D12.h"
#include "CommandBufferD3D12.h"
#include "DescriptorD3D12.h"
#include "DescriptorPoolD3D12.h"
#include "PipelineD3D12.h"
#include "PipelineLayoutD3D12.h"
#include "QueryPoolD3D12.h"
#include "TextureD3D12.h"

#include <pix.h>

using namespace nri;

static uint8_t QueryLatestGraphicsCommandList(ComPtr<ID3D12GraphicsCommandListBest>& in, ComPtr<ID3D12GraphicsCommandListBest>& out) {
    static const IID versions[] = {
#ifdef NRI_USE_AGILITY_SDK
        __uuidof(ID3D12GraphicsCommandList9), __uuidof(ID3D12GraphicsCommandList8), __uuidof(ID3D12GraphicsCommandList7),
#endif
        __uuidof(ID3D12GraphicsCommandList6), __uuidof(ID3D12GraphicsCommandList5), __uuidof(ID3D12GraphicsCommandList4), __uuidof(ID3D12GraphicsCommandList3),
        __uuidof(ID3D12GraphicsCommandList2), __uuidof(ID3D12GraphicsCommandList1), __uuidof(ID3D12GraphicsCommandList),
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

#ifdef NRI_USE_AGILITY_SDK
static inline D3D12_BARRIER_SYNC GetBarrierSyncFlags(StageBits stageBits) {
    // Check non-mask values first
    if (stageBits == StageBits::ALL)
        return D3D12_BARRIER_SYNC_ALL;

    if (stageBits == StageBits::NONE)
        return D3D12_BARRIER_SYNC_NONE;

    // Gather bits
    D3D12_BARRIER_SYNC flags = D3D12_BARRIER_SYNC_NONE;

    if (stageBits & StageBits::INDEX_INPUT)
        flags |= D3D12_BARRIER_SYNC_INDEX_INPUT;

    if (stageBits & (StageBits::VERTEX_SHADER | StageBits::TESS_CONTROL_SHADER | StageBits::TESS_EVALUATION_SHADER | StageBits::GEOMETRY_SHADER | StageBits::MESH_CONTROL_SHADER |
                     StageBits::MESH_EVALUATION_SHADER | StageBits::STREAM_OUTPUT))
        flags |= D3D12_BARRIER_SYNC_VERTEX_SHADING;

    if (stageBits & StageBits::FRAGMENT_SHADER)
        flags |= D3D12_BARRIER_SYNC_PIXEL_SHADING;

    if (stageBits & StageBits::DEPTH_STENCIL_ATTACHMENT)
        flags |= D3D12_BARRIER_SYNC_DEPTH_STENCIL;

    if (stageBits & StageBits::COLOR_ATTACHMENT)
        flags |= D3D12_BARRIER_SYNC_RENDER_TARGET;

    if (stageBits & StageBits::COMPUTE_SHADER)
        flags |= D3D12_BARRIER_SYNC_COMPUTE_SHADING;

    if (stageBits & StageBits::RAY_TRACING_SHADERS)
        flags |= D3D12_BARRIER_SYNC_RAYTRACING;

    if (stageBits & StageBits::INDIRECT)
        flags |= D3D12_BARRIER_SYNC_EXECUTE_INDIRECT;

    if (stageBits & StageBits::COPY)
        flags |= D3D12_BARRIER_SYNC_COPY;

    if (stageBits & StageBits::CLEAR_STORAGE)
        flags |= D3D12_BARRIER_SYNC_CLEAR_UNORDERED_ACCESS_VIEW;

    if (stageBits & StageBits::ACCELERATION_STRUCTURE)
        flags |= D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE | D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE |
                 D3D12_BARRIER_SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO;

    return flags;
}

static inline D3D12_BARRIER_ACCESS GetBarrierAccessFlags(AccessBits accessBits) {
    if (accessBits == AccessBits::UNKNOWN)
        return D3D12_BARRIER_ACCESS_NO_ACCESS;

    D3D12_BARRIER_ACCESS flags = D3D12_BARRIER_ACCESS_COMMON; // = 0

    if (accessBits & AccessBits::VERTEX_BUFFER)
        flags |= D3D12_BARRIER_ACCESS_VERTEX_BUFFER;

    if (accessBits & AccessBits::INDEX_BUFFER)
        flags |= D3D12_BARRIER_ACCESS_INDEX_BUFFER;

    if (accessBits & AccessBits::CONSTANT_BUFFER)
        flags |= D3D12_BARRIER_ACCESS_CONSTANT_BUFFER;

    if (accessBits & AccessBits::ARGUMENT_BUFFER)
        flags |= D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;

    if (accessBits & AccessBits::SHADER_RESOURCE)
        flags |= D3D12_BARRIER_ACCESS_SHADER_RESOURCE;

    if (accessBits & AccessBits::SHADER_RESOURCE_STORAGE)
        flags |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;

    if (accessBits & AccessBits::COLOR_ATTACHMENT)
        flags |= D3D12_BARRIER_ACCESS_RENDER_TARGET;

    if (accessBits & AccessBits::DEPTH_STENCIL_WRITE)
        flags |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;

    if (accessBits & AccessBits::DEPTH_STENCIL_READ)
        flags |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;

    if (accessBits & AccessBits::COPY_SOURCE)
        flags |= D3D12_BARRIER_ACCESS_COPY_SOURCE;

    if (accessBits & AccessBits::COPY_DESTINATION)
        flags |= D3D12_BARRIER_ACCESS_COPY_DEST;

    if (accessBits & AccessBits::ACCELERATION_STRUCTURE_READ)
        flags |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;

    if (accessBits & AccessBits::ACCELERATION_STRUCTURE_WRITE)
        flags |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;

    if (accessBits & AccessBits::STREAM_OUTPUT)
        flags |= D3D12_BARRIER_ACCESS_STREAM_OUTPUT;

    if (accessBits & AccessBits::SHADING_RATE)
        flags |= D3D12_BARRIER_ACCESS_SHADING_RATE_SOURCE;

    return flags;
}

constexpr std::array<D3D12_BARRIER_LAYOUT, (uint32_t)DescriptorType::MAX_NUM> LAYOUTS = {
    D3D12_BARRIER_LAYOUT_UNDEFINED,           // UNKNOWN
    D3D12_BARRIER_LAYOUT_RENDER_TARGET,       // COLOR_ATTACHMENT
    D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE, // DEPTH_STENCIL
    D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ,  // DEPTH_STENCIL_READONLY
    D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,     // SHADER_RESOURCE
    D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,    // SHADER_RESOURCE_STORAGE
    D3D12_BARRIER_LAYOUT_COPY_SOURCE,         // COPY_SOURCE
    D3D12_BARRIER_LAYOUT_COPY_DEST,           // COPY_DESTINATION
    D3D12_BARRIER_LAYOUT_PRESENT,             // PRESENT
};

static inline D3D12_BARRIER_LAYOUT GetBarrierLayout(Layout layout) {
    return LAYOUTS[(uint32_t)layout];
}
#endif

static inline D3D12_RESOURCE_STATES GetResourceStates(AccessBits accessMask, D3D12_COMMAND_LIST_TYPE commandListType) {
    D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_COMMON;

    if (accessMask & (AccessBits::CONSTANT_BUFFER | AccessBits::VERTEX_BUFFER))
        resourceStates |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    if (accessMask & AccessBits::INDEX_BUFFER)
        resourceStates |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
    if (accessMask & AccessBits::ARGUMENT_BUFFER)
        resourceStates |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    if (accessMask & AccessBits::SHADER_RESOURCE_STORAGE)
        resourceStates |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    if (accessMask & AccessBits::COLOR_ATTACHMENT)
        resourceStates |= D3D12_RESOURCE_STATE_RENDER_TARGET;
    if (accessMask & AccessBits::DEPTH_STENCIL_READ)
        resourceStates |= D3D12_RESOURCE_STATE_DEPTH_READ;
    if (accessMask & AccessBits::DEPTH_STENCIL_WRITE)
        resourceStates |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
    if (accessMask & AccessBits::COPY_SOURCE)
        resourceStates |= D3D12_RESOURCE_STATE_COPY_SOURCE;
    if (accessMask & AccessBits::COPY_DESTINATION)
        resourceStates |= D3D12_RESOURCE_STATE_COPY_DEST;
    if (accessMask & AccessBits::SHADER_RESOURCE) {
        resourceStates |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        if (commandListType == D3D12_COMMAND_LIST_TYPE_DIRECT)
            resourceStates |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
    if (accessMask & AccessBits::ACCELERATION_STRUCTURE_READ)
        resourceStates |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
    if (accessMask & AccessBits::ACCELERATION_STRUCTURE_WRITE)
        resourceStates |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    return resourceStates;
}

static void AddResourceBarrier(
    D3D12_COMMAND_LIST_TYPE commandListType, ID3D12Resource* resource, AccessBits before, AccessBits after, D3D12_RESOURCE_BARRIER& resourceBarrier, uint32_t subresource
) {
    D3D12_RESOURCE_STATES resourceStateBefore = GetResourceStates(before, commandListType);
    D3D12_RESOURCE_STATES resourceStateAfter = GetResourceStates(after, commandListType);

    if (resourceStateBefore == resourceStateAfter && resourceStateBefore == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
        resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        resourceBarrier.UAV.pResource = resource;
    } else {
        resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resourceBarrier.Transition.pResource = resource;
        resourceBarrier.Transition.StateBefore = resourceStateBefore;
        resourceBarrier.Transition.StateAfter = resourceStateAfter;
        resourceBarrier.Transition.Subresource = subresource;
    }
}

Result CommandBufferD3D12::Create(D3D12_COMMAND_LIST_TYPE commandListType, ID3D12CommandAllocator* commandAllocator) {
    ComPtr<ID3D12GraphicsCommandListBest> graphicsCommandList;
    HRESULT hr = m_Device->CreateCommandList(NRI_TEMP_NODE_MASK, commandListType, commandAllocator, nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)&graphicsCommandList);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommandList()");

    m_Version = QueryLatestGraphicsCommandList(graphicsCommandList, m_GraphicsCommandList);

    hr = m_GraphicsCommandList->Close();
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12GraphicsCommandList::Close()");

    m_CommandAllocator = commandAllocator;

    return Result::SUCCESS;
}

Result CommandBufferD3D12::Create(const CommandBufferD3D12Desc& commandBufferDesc) {
    ComPtr<ID3D12GraphicsCommandListBest> graphicsCommandList = (ID3D12GraphicsCommandListBest*)commandBufferDesc.d3d12CommandList;
    m_Version = QueryLatestGraphicsCommandList(graphicsCommandList, m_GraphicsCommandList);

    // TODO: what if opened?

    m_CommandAllocator = commandBufferDesc.d3d12CommandAllocator;

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline Result CommandBufferD3D12::Begin(const DescriptorPool* descriptorPool) {
    HRESULT hr = m_GraphicsCommandList->Reset(m_CommandAllocator, nullptr);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12GraphicsCommandList::Reset()");

    if (descriptorPool)
        SetDescriptorPool(*descriptorPool);

    m_PipelineLayout = nullptr;
    m_IsGraphicsPipelineLayout = false;
    m_Pipeline = nullptr;
    m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

    return Result::SUCCESS;
}

inline Result CommandBufferD3D12::End() {
    if (FAILED(m_GraphicsCommandList->Close()))
        return Result::FAILURE;

    return Result::SUCCESS;
}

inline void CommandBufferD3D12::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    static_assert(offsetof(Viewport, x) == 0, "Unsupported viewport data layout.");
    static_assert(offsetof(Viewport, width) == 8, "Unsupported viewport data layout.");
    static_assert(offsetof(Viewport, depthRangeMin) == 16, "Unsupported viewport data layout.");
    static_assert(offsetof(Viewport, depthRangeMax) == 20, "Unsupported viewport data layout.");

    m_GraphicsCommandList->RSSetViewports(viewportNum, (D3D12_VIEWPORT*)viewports);
}

inline void CommandBufferD3D12::SetScissors(const Rect* rects, uint32_t rectNum) {
    D3D12_RECT* rectsD3D12 = STACK_ALLOC(D3D12_RECT, rectNum);
    ConvertRects(rectsD3D12, rects, rectNum);

    m_GraphicsCommandList->RSSetScissorRects(rectNum, rectsD3D12);
}

inline void CommandBufferD3D12::SetDepthBounds(float boundsMin, float boundsMax) {
    if (m_Version >= 1)
        m_GraphicsCommandList->OMSetDepthBounds(boundsMin, boundsMax);
}

inline void CommandBufferD3D12::SetStencilReference(uint8_t reference) {
    m_GraphicsCommandList->OMSetStencilRef(reference);
}

inline void CommandBufferD3D12::SetSamplePositions(const SamplePosition* positions, uint32_t positionNum) {
    Sample_t sampleNum = m_Pipeline->GetSampleNum();
    uint32_t pixelNum = positionNum / sampleNum;

    static_assert(sizeof(D3D12_SAMPLE_POSITION) == sizeof(SamplePosition));
    if (m_Version >= 1)
        m_GraphicsCommandList->SetSamplePositions(sampleNum, pixelNum, (D3D12_SAMPLE_POSITION*)positions);
}

inline void CommandBufferD3D12::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    D3D12_RECT* rectsD3D12 = STACK_ALLOC(D3D12_RECT, rectNum);
    ConvertRects(rectsD3D12, rects, rectNum);

    for (uint32_t i = 0; i < clearDescNum; i++) {
        if (AttachmentContentType::COLOR == clearDescs[i].attachmentContentType)
            m_GraphicsCommandList->ClearRenderTargetView(m_RenderTargets[clearDescs[i].colorAttachmentIndex], &clearDescs[i].value.color32f.x, rectNum, rectsD3D12);
        else if (m_DepthStencil.ptr) {
            D3D12_CLEAR_FLAGS clearFlags = (D3D12_CLEAR_FLAGS)0;
            switch (clearDescs[i].attachmentContentType) {
                case AttachmentContentType::DEPTH:
                    clearFlags = D3D12_CLEAR_FLAG_DEPTH;
                    break;
                case AttachmentContentType::STENCIL:
                    clearFlags = D3D12_CLEAR_FLAG_STENCIL;
                    break;
                case AttachmentContentType::DEPTH_STENCIL:
                    clearFlags = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL;
                    break;
            }

            m_GraphicsCommandList->ClearDepthStencilView(
                m_DepthStencil, clearFlags, clearDescs[i].value.depthStencil.depth, clearDescs[i].value.depthStencil.stencil, rectNum, rectsD3D12
            );
        }
    }
}

inline void CommandBufferD3D12::ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc) {
    DescriptorSetD3D12* descriptorSet = m_DescriptorSets[clearDesc.setIndexInPipelineLayout];
    DescriptorD3D12* resourceView = (DescriptorD3D12*)clearDesc.storageBuffer;
    const UINT clearValues[4] = {clearDesc.value, clearDesc.value, clearDesc.value, clearDesc.value};

    m_GraphicsCommandList->ClearUnorderedAccessViewUint(
        {descriptorSet->GetPointerGPU(clearDesc.rangeIndex, clearDesc.offsetInRange)}, {resourceView->GetPointerCPU()}, *resourceView, clearValues, 0, nullptr
    );
}

inline void CommandBufferD3D12::ClearStorageTexture(const ClearStorageTextureDesc& clearDesc) {
    DescriptorSetD3D12* descriptorSet = m_DescriptorSets[clearDesc.setIndexInPipelineLayout];
    DescriptorD3D12* resourceView = (DescriptorD3D12*)clearDesc.storageTexture;

    if (resourceView->IsIntegerFormat()) {
        m_GraphicsCommandList->ClearUnorderedAccessViewUint(
            {descriptorSet->GetPointerGPU(clearDesc.rangeIndex, clearDesc.offsetInRange)}, {resourceView->GetPointerCPU()}, *resourceView, &clearDesc.value.color32ui.x, 0, nullptr
        );
    } else {
        m_GraphicsCommandList->ClearUnorderedAccessViewFloat(
            {descriptorSet->GetPointerGPU(clearDesc.rangeIndex, clearDesc.offsetInRange)}, {resourceView->GetPointerCPU()}, *resourceView, &clearDesc.value.color32f.x, 0, nullptr
        );
    }
}

inline void CommandBufferD3D12::BeginRendering(const AttachmentsDesc& attachmentsDesc) {
    m_RenderTargetNum = attachmentsDesc.colors ? attachmentsDesc.colorNum : 0;

    uint32_t i = 0;
    for (; i < m_RenderTargetNum; i++) {
        const DescriptorD3D12& descriptor = *(DescriptorD3D12*)attachmentsDesc.colors[i];
        m_RenderTargets[i].ptr = descriptor.GetPointerCPU();
    }
    for (; i < (uint32_t)m_RenderTargets.size(); i++)
        m_RenderTargets[i].ptr = NULL;

    if (attachmentsDesc.depthStencil) {
        const DescriptorD3D12& descriptor = *(DescriptorD3D12*)attachmentsDesc.depthStencil;
        m_DepthStencil.ptr = descriptor.GetPointerCPU();
    } else
        m_DepthStencil.ptr = NULL;

    m_GraphicsCommandList->OMSetRenderTargets(m_RenderTargetNum, m_RenderTargets.data(), FALSE, m_DepthStencil.ptr ? &m_DepthStencil : nullptr);
}

inline void CommandBufferD3D12::SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    D3D12_VERTEX_BUFFER_VIEW* vertexBufferViews = STACK_ALLOC(D3D12_VERTEX_BUFFER_VIEW, bufferNum);

    for (uint32_t i = 0; i < bufferNum; i++) {
        if (buffers[i] != nullptr) {
            const BufferD3D12* buffer = (BufferD3D12*)buffers[i];
            uint64_t offset = offsets ? offsets[i] : 0;
            vertexBufferViews[i].BufferLocation = buffer->GetPointerGPU() + offset;
            vertexBufferViews[i].SizeInBytes = (UINT)(buffer->GetDesc().size - offset);
            vertexBufferViews[i].StrideInBytes = m_Pipeline->GetIAStreamStride(baseSlot + i);
        } else {
            vertexBufferViews[i].BufferLocation = 0;
            vertexBufferViews[i].SizeInBytes = 0;
            vertexBufferViews[i].StrideInBytes = 0;
        }
    }

    m_GraphicsCommandList->IASetVertexBuffers(baseSlot, bufferNum, vertexBufferViews);
}

inline void CommandBufferD3D12::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    const BufferD3D12& bufferD3D12 = (BufferD3D12&)buffer;

    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    indexBufferView.BufferLocation = bufferD3D12.GetPointerGPU() + offset;
    indexBufferView.SizeInBytes = (UINT)(bufferD3D12.GetDesc().size - offset);
    indexBufferView.Format = indexType == IndexType::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

    m_GraphicsCommandList->IASetIndexBuffer(&indexBufferView);
}

inline void CommandBufferD3D12::SetPipelineLayout(const PipelineLayout& pipelineLayout) {
    const PipelineLayoutD3D12& pipelineLayoutD3D12 = (const PipelineLayoutD3D12&)pipelineLayout;

    if (m_PipelineLayout == &pipelineLayoutD3D12)
        return;

    m_PipelineLayout = &pipelineLayoutD3D12;
    m_IsGraphicsPipelineLayout = pipelineLayoutD3D12.IsGraphicsPipelineLayout();

    if (m_IsGraphicsPipelineLayout)
        m_GraphicsCommandList->SetGraphicsRootSignature(pipelineLayoutD3D12);
    else
        m_GraphicsCommandList->SetComputeRootSignature(pipelineLayoutD3D12);
}

inline void CommandBufferD3D12::SetPipeline(const Pipeline& pipeline) {
    PipelineD3D12* pipelineD3D12 = (PipelineD3D12*)&pipeline;

    if (m_Pipeline == pipelineD3D12)
        return;

    pipelineD3D12->Bind(m_GraphicsCommandList, m_PrimitiveTopology);

    m_Pipeline = pipelineD3D12;
}

inline void CommandBufferD3D12::SetDescriptorPool(const DescriptorPool& descriptorPool) {
    ((DescriptorPoolD3D12&)descriptorPool).Bind(m_GraphicsCommandList);
}

inline void CommandBufferD3D12::SetDescriptorSet(uint32_t setIndexInPipelineLayout, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    m_PipelineLayout->SetDescriptorSet(*m_GraphicsCommandList, m_IsGraphicsPipelineLayout, setIndexInPipelineLayout, descriptorSet, dynamicConstantBufferOffsets);
    m_DescriptorSets[setIndexInPipelineLayout] = (DescriptorSetD3D12*)&descriptorSet;
}

inline void CommandBufferD3D12::SetConstants(uint32_t pushConstantRangeIndex, const void* data, uint32_t size) {
    uint32_t rootParameterIndex = m_PipelineLayout->GetPushConstantsRootOffset(pushConstantRangeIndex);
    uint32_t constantNum = size / 4;

    if (m_IsGraphicsPipelineLayout)
        m_GraphicsCommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, constantNum, data, 0);
    else
        m_GraphicsCommandList->SetComputeRoot32BitConstants(rootParameterIndex, constantNum, data, 0);
}

inline void CommandBufferD3D12::Draw(uint32_t vertexNum, uint32_t instanceNum, uint32_t baseVertex, uint32_t baseInstance) {
    m_GraphicsCommandList->DrawInstanced(vertexNum, instanceNum, baseVertex, baseInstance);
}

inline void CommandBufferD3D12::DrawIndexed(uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance) {
    m_GraphicsCommandList->DrawIndexedInstanced(indexNum, instanceNum, baseIndex, baseVertex, baseInstance);
}

inline void CommandBufferD3D12::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride) {
    m_GraphicsCommandList->ExecuteIndirect(m_Device.GetDrawCommandSignature(stride), drawNum, (BufferD3D12&)buffer, offset, nullptr, 0);
}

inline void CommandBufferD3D12::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride) {
    m_GraphicsCommandList->ExecuteIndirect(m_Device.GetDrawIndexedCommandSignature(stride), drawNum, (BufferD3D12&)buffer, offset, nullptr, 0);
}

inline void CommandBufferD3D12::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    if (size == WHOLE_SIZE)
        size = ((BufferD3D12&)srcBuffer).GetDesc().size;

    m_GraphicsCommandList->CopyBufferRegion((BufferD3D12&)dstBuffer, dstOffset, (BufferD3D12&)srcBuffer, srcOffset, size);
}

inline void CommandBufferD3D12::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    TextureD3D12& dstTextureD3D12 = (TextureD3D12&)dstTexture;
    TextureD3D12& srcTextureD3D12 = (TextureD3D12&)srcTexture;

    if (!dstRegion || !srcRegion) {
        m_GraphicsCommandList->CopyResource(dstTextureD3D12, srcTextureD3D12);
    } else {
        D3D12_TEXTURE_COPY_LOCATION dstTextureCopyLocation = {
            dstTextureD3D12, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, dstTextureD3D12.GetSubresourceIndex(dstRegion->arrayOffset, dstRegion->mipOffset)
        };

        D3D12_TEXTURE_COPY_LOCATION srcTextureCopyLocation = {
            srcTextureD3D12, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, srcTextureD3D12.GetSubresourceIndex(srcRegion->arrayOffset, srcRegion->mipOffset)
        };

        const uint16_t size[3] = {
            srcRegion->width == WHOLE_SIZE ? srcTextureD3D12.GetSize(0, srcRegion->mipOffset) : srcRegion->width,
            srcRegion->height == WHOLE_SIZE ? srcTextureD3D12.GetSize(1, srcRegion->mipOffset) : srcRegion->height,
            srcRegion->depth == WHOLE_SIZE ? srcTextureD3D12.GetSize(2, srcRegion->mipOffset) : srcRegion->depth
        };
        D3D12_BOX box = {srcRegion->x, srcRegion->y, srcRegion->z, uint16_t(srcRegion->x + size[0]), uint16_t(srcRegion->y + size[1]), uint16_t(srcRegion->z + size[2])};

        m_GraphicsCommandList->CopyTextureRegion(&dstTextureCopyLocation, dstRegion->x, dstRegion->y, dstRegion->z, &srcTextureCopyLocation, &box);
    }
}

inline void CommandBufferD3D12::UploadBufferToTexture(
    Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc
) {
    TextureD3D12& dstTextureD3D12 = (TextureD3D12&)dstTexture;
    D3D12_TEXTURE_COPY_LOCATION dstTextureCopyLocation = {
        dstTextureD3D12, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, dstTextureD3D12.GetSubresourceIndex(dstRegionDesc.arrayOffset, dstRegionDesc.mipOffset)
    };

    const uint16_t size[3] = {
        dstRegionDesc.width == WHOLE_SIZE ? dstTextureD3D12.GetSize(0, dstRegionDesc.mipOffset) : dstRegionDesc.width,
        dstRegionDesc.height == WHOLE_SIZE ? dstTextureD3D12.GetSize(1, dstRegionDesc.mipOffset) : dstRegionDesc.height,
        dstRegionDesc.depth == WHOLE_SIZE ? dstTextureD3D12.GetSize(2, dstRegionDesc.mipOffset) : dstRegionDesc.depth
    };

    D3D12_TEXTURE_COPY_LOCATION srcTextureCopyLocation;
    srcTextureCopyLocation.pResource = (BufferD3D12&)srcBuffer;
    srcTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcTextureCopyLocation.PlacedFootprint.Offset = srcDataLayoutDesc.offset;
    srcTextureCopyLocation.PlacedFootprint.Footprint.Format = GetDxgiFormat(dstTextureD3D12.GetDesc().format).typeless;
    srcTextureCopyLocation.PlacedFootprint.Footprint.Width = size[0];
    srcTextureCopyLocation.PlacedFootprint.Footprint.Height = size[1];
    srcTextureCopyLocation.PlacedFootprint.Footprint.Depth = size[2];
    srcTextureCopyLocation.PlacedFootprint.Footprint.RowPitch = srcDataLayoutDesc.rowPitch;

    D3D12_BOX box = {
        dstRegionDesc.x, dstRegionDesc.y, dstRegionDesc.z, uint16_t(dstRegionDesc.x + size[0]), uint16_t(dstRegionDesc.y + size[1]), uint16_t(dstRegionDesc.z + size[2])
    };

    m_GraphicsCommandList->CopyTextureRegion(&dstTextureCopyLocation, dstRegionDesc.x, dstRegionDesc.y, dstRegionDesc.z, &srcTextureCopyLocation, &box);
}

inline void CommandBufferD3D12::ReadbackTextureToBuffer(
    Buffer& dstBuffer, TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc
) {
    TextureD3D12& srcTextureD3D12 = (TextureD3D12&)srcTexture;
    D3D12_TEXTURE_COPY_LOCATION srcTextureCopyLocation = {
        srcTextureD3D12, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, srcTextureD3D12.GetSubresourceIndex(srcRegionDesc.arrayOffset, srcRegionDesc.mipOffset)
    };

    D3D12_TEXTURE_COPY_LOCATION dstTextureCopyLocation;
    dstTextureCopyLocation.pResource = (BufferD3D12&)dstBuffer;
    dstTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstTextureCopyLocation.PlacedFootprint.Offset = dstDataLayoutDesc.offset;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Format = GetDxgiFormat(srcTextureD3D12.GetDesc().format).typeless;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Width = srcRegionDesc.width;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Height = srcRegionDesc.height;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Depth = srcRegionDesc.depth;
    dstTextureCopyLocation.PlacedFootprint.Footprint.RowPitch = dstDataLayoutDesc.rowPitch;

    const uint16_t size[3] = {
        srcRegionDesc.width == WHOLE_SIZE ? srcTextureD3D12.GetSize(0, srcRegionDesc.mipOffset) : srcRegionDesc.width,
        srcRegionDesc.height == WHOLE_SIZE ? srcTextureD3D12.GetSize(1, srcRegionDesc.mipOffset) : srcRegionDesc.height,
        srcRegionDesc.depth == WHOLE_SIZE ? srcTextureD3D12.GetSize(2, srcRegionDesc.mipOffset) : srcRegionDesc.depth
    };
    D3D12_BOX box = {
        srcRegionDesc.x, srcRegionDesc.y, srcRegionDesc.z, uint16_t(srcRegionDesc.x + size[0]), uint16_t(srcRegionDesc.y + size[1]), uint16_t(srcRegionDesc.z + size[2])
    };

    m_GraphicsCommandList->CopyTextureRegion(&dstTextureCopyLocation, 0, 0, 0, &srcTextureCopyLocation, &box);
}

inline void CommandBufferD3D12::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
    m_GraphicsCommandList->Dispatch(x, y, z);
}

inline void CommandBufferD3D12::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    m_GraphicsCommandList->ExecuteIndirect(m_Device.GetDispatchCommandSignature(), 1, (BufferD3D12&)buffer, offset, nullptr, 0);
}

inline void CommandBufferD3D12::Barrier(const BarrierGroupDesc& barrierGroupDesc) {
#ifdef NRI_USE_AGILITY_SDK
    if (m_Device.AreEnhancedBarriersSupported()) { // Enhanced barriers
        // Count
        uint32_t barrierNum = barrierGroupDesc.globalNum + barrierGroupDesc.bufferNum + barrierGroupDesc.textureNum;
        if (!barrierNum)
            return;

        D3D12_BARRIER_GROUP barrierGroups[3] = {};
        uint32_t barriersGroupsNum = 0;

        // Global
        uint16_t num = barrierGroupDesc.globalNum;
        D3D12_GLOBAL_BARRIER* globalBarriers = STACK_ALLOC(D3D12_GLOBAL_BARRIER, num);
        if (num) {
            D3D12_BARRIER_GROUP* barrierGroup = &barrierGroups[barriersGroupsNum++];
            barrierGroup->Type = D3D12_BARRIER_TYPE_GLOBAL;
            barrierGroup->NumBarriers = num;
            barrierGroup->pGlobalBarriers = globalBarriers;

            for (uint16_t i = 0; i < num; i++) {
                const GlobalBarrierDesc& in = barrierGroupDesc.globals[i];

                D3D12_GLOBAL_BARRIER& out = globalBarriers[i];
                out.SyncBefore = GetBarrierSyncFlags(in.before.stages);
                out.SyncAfter = GetBarrierSyncFlags(in.after.stages);
                out.AccessBefore = GetBarrierAccessFlags(in.before.access);
                out.AccessAfter = GetBarrierAccessFlags(in.after.access);
            }
        }

        // Buffer
        num = barrierGroupDesc.bufferNum;
        D3D12_BUFFER_BARRIER* bufferBarriers = STACK_ALLOC(D3D12_BUFFER_BARRIER, num);
        if (barrierGroupDesc.bufferNum) {
            D3D12_BARRIER_GROUP* barrierGroup = &barrierGroups[barriersGroupsNum++];
            barrierGroup->Type = D3D12_BARRIER_TYPE_BUFFER;
            barrierGroup->NumBarriers = num;
            barrierGroup->pBufferBarriers = bufferBarriers;

            for (uint16_t i = 0; i < num; i++) {
                const BufferBarrierDesc& in = barrierGroupDesc.buffers[i];
                const BufferD3D12& buffer = *(BufferD3D12*)in.buffer;

                D3D12_BUFFER_BARRIER& out = bufferBarriers[i];
                out.SyncBefore = GetBarrierSyncFlags(in.before.stages);
                out.SyncAfter = GetBarrierSyncFlags(in.after.stages);
                out.AccessBefore = GetBarrierAccessFlags(in.before.access);
                out.AccessAfter = GetBarrierAccessFlags(in.after.access);
                out.pResource = buffer;
                out.Offset = 0;
                out.Size = UINT64_MAX;
            }
        }

        // Texture
        num = barrierGroupDesc.textureNum;
        D3D12_TEXTURE_BARRIER* textureBarriers = STACK_ALLOC(D3D12_TEXTURE_BARRIER, num);
        if (barrierGroupDesc.textureNum) {
            D3D12_BARRIER_GROUP* barrierGroup = &barrierGroups[barriersGroupsNum++];
            barrierGroup->Type = D3D12_BARRIER_TYPE_TEXTURE;
            barrierGroup->NumBarriers = num;
            barrierGroup->pTextureBarriers = textureBarriers;

            memset(textureBarriers, 0, sizeof(D3D12_TEXTURE_BARRIER) * num);
            for (uint16_t i = 0; i < num; i++) {
                const TextureBarrierDesc& in = barrierGroupDesc.textures[i];
                const TextureD3D12& texture = *(TextureD3D12*)in.texture;
                const TextureDesc& desc = texture.GetDesc();

                D3D12_TEXTURE_BARRIER& out = textureBarriers[i];
                out.SyncBefore = GetBarrierSyncFlags(in.before.stages);
                out.SyncAfter = GetBarrierSyncFlags(in.after.stages);
                out.AccessBefore = in.before.layout == Layout::PRESENT ? D3D12_BARRIER_ACCESS_COMMON : GetBarrierAccessFlags(in.before.access);
                out.AccessAfter = in.after.layout == Layout::PRESENT ? D3D12_BARRIER_ACCESS_COMMON : GetBarrierAccessFlags(in.after.access);
                out.LayoutBefore = GetBarrierLayout(in.before.layout);
                out.LayoutAfter = GetBarrierLayout(in.after.layout);
                out.pResource = texture;
                out.Subresources.IndexOrFirstMipLevel = in.mipOffset;
                out.Subresources.NumMipLevels = in.mipNum == REMAINING_MIP_LEVELS ? desc.mipNum : in.mipNum;
                out.Subresources.FirstArraySlice = in.arrayOffset;
                out.Subresources.NumArraySlices = in.arraySize == REMAINING_ARRAY_LAYERS ? desc.arraySize : in.arraySize;
                out.Subresources.FirstPlane = 0;
                out.Subresources.NumPlanes = 1;

                // https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#d3d12_texture_barrier_flags
                out.Flags = in.before.layout == Layout::UNKNOWN ? D3D12_TEXTURE_BARRIER_FLAG_DISCARD : D3D12_TEXTURE_BARRIER_FLAG_NONE; // TODO: verify that it works
            }
        }

        // Submit
        m_GraphicsCommandList->Barrier(barriersGroupsNum, barrierGroups);
    } else
#endif
    { // Legacy barriers
        // Count
        uint32_t barrierNum = barrierGroupDesc.bufferNum;

        for (uint32_t i = 0; i < barrierGroupDesc.textureNum; i++) {
            const TextureBarrierDesc& barrierDesc = barrierGroupDesc.textures[i];
            const TextureD3D12& texture = *(TextureD3D12*)barrierDesc.texture;
            const TextureDesc& textureDesc = texture.GetDesc();
            const Dim_t arraySize = barrierDesc.arraySize == REMAINING_ARRAY_LAYERS ? textureDesc.arraySize : barrierDesc.arraySize;
            const Mip_t mipNum = barrierDesc.mipNum == REMAINING_MIP_LEVELS ? textureDesc.mipNum : barrierDesc.mipNum;

            if (barrierDesc.arrayOffset == 0 && arraySize == textureDesc.arraySize && barrierDesc.mipOffset == 0 && mipNum == textureDesc.mipNum)
                barrierNum++;
            else
                barrierNum += arraySize * mipNum;
        }

        bool isGlobalUavBarrierNeeded = false;
        for (uint32_t i = 0; i < barrierGroupDesc.globalNum && !isGlobalUavBarrierNeeded; i++) {
            const GlobalBarrierDesc& barrierDesc = barrierGroupDesc.globals[i];
            if (barrierDesc.before.access == barrierDesc.after.access && (barrierDesc.before.access & AccessBits::SHADER_RESOURCE_STORAGE))
                isGlobalUavBarrierNeeded = true;
        }

        if (isGlobalUavBarrierNeeded)
            barrierNum++;

        if (!barrierNum)
            return;

        // Gather
        D3D12_RESOURCE_BARRIER* barriers = STACK_ALLOC(D3D12_RESOURCE_BARRIER, barrierNum);
        memset(barriers, 0, sizeof(D3D12_RESOURCE_BARRIER) * barrierNum);

        D3D12_RESOURCE_BARRIER* ptr = barriers;
        D3D12_COMMAND_LIST_TYPE commandListType = m_GraphicsCommandList->GetType();

        for (uint32_t i = 0; i < barrierGroupDesc.bufferNum; i++) {
            const BufferBarrierDesc& barrierDesc = barrierGroupDesc.buffers[i];
            AddResourceBarrier(commandListType, *((BufferD3D12*)barrierDesc.buffer), barrierDesc.before.access, barrierDesc.after.access, *ptr++, 0);
        }

        for (uint32_t i = 0; i < barrierGroupDesc.textureNum; i++) {
            const TextureBarrierDesc& barrierDesc = barrierGroupDesc.textures[i];
            const TextureD3D12& texture = *(TextureD3D12*)barrierDesc.texture;
            const TextureDesc& textureDesc = texture.GetDesc();
            const Dim_t arraySize = barrierDesc.arraySize == REMAINING_ARRAY_LAYERS ? textureDesc.arraySize : barrierDesc.arraySize;
            const Mip_t mipNum = barrierDesc.mipNum == REMAINING_MIP_LEVELS ? textureDesc.mipNum : barrierDesc.mipNum;

            if (barrierDesc.arrayOffset == 0 && arraySize == textureDesc.arraySize && barrierDesc.mipOffset == 0 && mipNum == textureDesc.mipNum)
                AddResourceBarrier(commandListType, texture, barrierDesc.before.access, barrierDesc.after.access, *ptr++, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
            else {
                for (Dim_t arrayOffset = barrierDesc.arrayOffset; arrayOffset < barrierDesc.arrayOffset + arraySize; arrayOffset++) {
                    for (Mip_t mipOffset = barrierDesc.mipOffset; mipOffset < barrierDesc.mipOffset + mipNum; mipOffset++) {
                        uint32_t subresource = texture.GetSubresourceIndex(arrayOffset, mipOffset);
                        AddResourceBarrier(commandListType, texture, barrierDesc.before.access, barrierDesc.after.access, *ptr++, subresource);
                    }
                }
            }
        }

        if (isGlobalUavBarrierNeeded) {
            ptr->Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            ptr->UAV.pResource = nullptr;
        }

        // Submit
        m_GraphicsCommandList->ResourceBarrier(barrierNum, barriers);
    }
}

inline void CommandBufferD3D12::BeginQuery(const QueryPool& queryPool, uint32_t offset) {
    const QueryPoolD3D12& queryPoolD3D12 = (QueryPoolD3D12&)queryPool;
    m_GraphicsCommandList->BeginQuery(queryPoolD3D12, queryPoolD3D12.GetType(), offset);
}

inline void CommandBufferD3D12::EndQuery(const QueryPool& queryPool, uint32_t offset) {
    const QueryPoolD3D12& queryPoolD3D12 = (QueryPoolD3D12&)queryPool;
    m_GraphicsCommandList->EndQuery(queryPoolD3D12, queryPoolD3D12.GetType(), offset);
}

inline void CommandBufferD3D12::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& buffer, uint64_t alignedBufferOffset) {
    const QueryPoolD3D12& queryPoolD3D12 = (QueryPoolD3D12&)queryPool;
    const BufferD3D12& bufferD3D12 = (BufferD3D12&)buffer;

    if (queryPoolD3D12.GetType() == QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE) {
        const uint64_t srcOffset = offset * queryPoolD3D12.GetQuerySize();
        const uint64_t size = num * queryPoolD3D12.GetQuerySize();
        m_GraphicsCommandList->CopyBufferRegion(bufferD3D12, alignedBufferOffset, queryPoolD3D12.GetReadbackBuffer(), srcOffset, size);
        return;
    }

    m_GraphicsCommandList->ResolveQueryData(queryPoolD3D12, queryPoolD3D12.GetType(), offset, num, bufferD3D12, alignedBufferOffset);
}

inline void CommandBufferD3D12::BeginAnnotation(const char* name) {
    size_t len = strlen(name) + 1;
    wchar_t* s = STACK_ALLOC(wchar_t, len);
    ConvertCharToWchar(name, s, len);

    PIXBeginEvent(m_GraphicsCommandList, PIX_COLOR_DEFAULT, s);
}

inline void CommandBufferD3D12::EndAnnotation() {
    PIXEndEvent(m_GraphicsCommandList);
}

inline void CommandBufferD3D12::BuildTopLevelAccelerationStructure(
    uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset
) {
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
    desc.DestAccelerationStructureData = ((AccelerationStructureD3D12&)dst).GetHandle();
    desc.ScratchAccelerationStructureData = ((BufferD3D12&)scratch).GetPointerGPU() + scratchOffset;
    desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    desc.Inputs.Flags = GetAccelerationStructureBuildFlags(flags);
    desc.Inputs.NumDescs = instanceNum;
    desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    desc.Inputs.InstanceDescs = ((BufferD3D12&)buffer).GetPointerGPU() + bufferOffset;

    if (m_Version >= 4)
        m_GraphicsCommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
}

inline void CommandBufferD3D12::BuildBottomLevelAccelerationStructure(
    uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset
) {
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
    desc.DestAccelerationStructureData = ((AccelerationStructureD3D12&)dst).GetHandle();
    desc.ScratchAccelerationStructureData = ((BufferD3D12&)scratch).GetPointerGPU() + scratchOffset;
    desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    desc.Inputs.Flags = GetAccelerationStructureBuildFlags(flags);
    desc.Inputs.NumDescs = geometryObjectNum;
    desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    Vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs(geometryObjectNum, m_Device.GetStdAllocator());
    ConvertGeometryDescs(&geometryDescs[0], geometryObjects, geometryObjectNum);
    desc.Inputs.pGeometryDescs = &geometryDescs[0];

    if (m_Version >= 4)
        m_GraphicsCommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
}

inline void CommandBufferD3D12::UpdateTopLevelAccelerationStructure(
    uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, AccelerationStructure& src,
    Buffer& scratch, uint64_t scratchOffset
) {
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
    desc.DestAccelerationStructureData = ((AccelerationStructureD3D12&)dst).GetHandle();
    desc.SourceAccelerationStructureData = ((AccelerationStructureD3D12&)src).GetHandle();
    desc.ScratchAccelerationStructureData = ((BufferD3D12&)scratch).GetPointerGPU() + scratchOffset;
    desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    desc.Inputs.Flags = GetAccelerationStructureBuildFlags(flags) | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    desc.Inputs.NumDescs = instanceNum;
    desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    desc.Inputs.InstanceDescs = ((BufferD3D12&)buffer).GetPointerGPU() + bufferOffset;

    if (m_Version >= 4)
        m_GraphicsCommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
}

inline void CommandBufferD3D12::UpdateBottomLevelAccelerationStructure(
    uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, AccelerationStructure& src,
    Buffer& scratch, uint64_t scratchOffset
) {
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
    desc.DestAccelerationStructureData = ((AccelerationStructureD3D12&)dst).GetHandle();
    desc.SourceAccelerationStructureData = ((AccelerationStructureD3D12&)src).GetHandle();
    desc.ScratchAccelerationStructureData = ((BufferD3D12&)scratch).GetPointerGPU() + scratchOffset;
    desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    desc.Inputs.Flags = GetAccelerationStructureBuildFlags(flags) | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    desc.Inputs.NumDescs = geometryObjectNum;
    desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    Vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs(geometryObjectNum, m_Device.GetStdAllocator());
    ConvertGeometryDescs(&geometryDescs[0], geometryObjects, geometryObjectNum);
    desc.Inputs.pGeometryDescs = &geometryDescs[0];

    if (m_Version >= 4)
        m_GraphicsCommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
}

inline void CommandBufferD3D12::CopyAccelerationStructure(AccelerationStructure& dst, AccelerationStructure& src, CopyMode copyMode) {
    m_GraphicsCommandList->CopyRaytracingAccelerationStructure(
        ((AccelerationStructureD3D12&)dst).GetHandle(), ((AccelerationStructureD3D12&)src).GetHandle(), GetCopyMode(copyMode)
    );
}

inline void CommandBufferD3D12::WriteAccelerationStructureSize(
    const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryOffset
) {
    D3D12_GPU_VIRTUAL_ADDRESS* virtualAddresses = ALLOCATE_SCRATCH(m_Device, D3D12_GPU_VIRTUAL_ADDRESS, accelerationStructureNum);
    for (uint32_t i = 0; i < accelerationStructureNum; i++)
        virtualAddresses[i] = ((AccelerationStructureD3D12&)accelerationStructures[i]).GetHandle();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postbuildInfo = {};
    postbuildInfo.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;
    postbuildInfo.DestBuffer = ((QueryPoolD3D12&)queryPool).GetReadbackBuffer()->GetGPUVirtualAddress() + queryOffset;

    if (m_Version >= 4)
        m_GraphicsCommandList->EmitRaytracingAccelerationStructurePostbuildInfo(&postbuildInfo, accelerationStructureNum, virtualAddresses);

    FREE_SCRATCH(m_Device, virtualAddresses, accelerationStructureNum);
}

inline void CommandBufferD3D12::DispatchRays(const DispatchRaysDesc& dispatchRaysDesc) {
    D3D12_DISPATCH_RAYS_DESC desc = {};

    desc.RayGenerationShaderRecord.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.raygenShader.buffer).GetPointerGPU() + dispatchRaysDesc.raygenShader.offset;
    desc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    if (dispatchRaysDesc.missShaders.buffer) {
        desc.MissShaderTable.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.missShaders.buffer).GetPointerGPU() + dispatchRaysDesc.missShaders.offset;
        desc.MissShaderTable.SizeInBytes = dispatchRaysDesc.missShaders.size;
        desc.MissShaderTable.StrideInBytes = dispatchRaysDesc.missShaders.stride;
    }

    if (dispatchRaysDesc.hitShaderGroups.buffer) {
        desc.HitGroupTable.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.hitShaderGroups.buffer).GetPointerGPU() + dispatchRaysDesc.hitShaderGroups.offset;
        desc.HitGroupTable.SizeInBytes = dispatchRaysDesc.hitShaderGroups.size;
        desc.HitGroupTable.StrideInBytes = dispatchRaysDesc.hitShaderGroups.stride;
    }

    if (dispatchRaysDesc.callableShaders.buffer) {
        desc.CallableShaderTable.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.callableShaders.buffer).GetPointerGPU() + dispatchRaysDesc.callableShaders.offset;
        desc.CallableShaderTable.SizeInBytes = dispatchRaysDesc.callableShaders.size;
        desc.CallableShaderTable.StrideInBytes = dispatchRaysDesc.callableShaders.stride;
    }

    desc.Width = dispatchRaysDesc.width;
    desc.Height = dispatchRaysDesc.height;
    desc.Depth = dispatchRaysDesc.depth;

    if (m_Version >= 4)
        m_GraphicsCommandList->DispatchRays(&desc);
}

inline void CommandBufferD3D12::DispatchMeshTasks(uint32_t x, uint32_t y, uint32_t z) {
    if (m_Version >= 6)
        m_GraphicsCommandList->DispatchMesh(x, y, z);
}

#include "CommandBufferD3D12.hpp"
