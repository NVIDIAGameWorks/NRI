// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"

#include "BufferD3D11.h"
#include "CommandBufferD3D11.h"
#include "DescriptorD3D11.h"
#include "DescriptorSetD3D11.h"
#include "PipelineD3D11.h"
#include "PipelineLayoutD3D11.h"
#include "QueryPoolD3D11.h"
#include "TextureD3D11.h"

#include "NRICompatibility.hlsli"

using namespace nri;

static constexpr uint64_t s_nullOffsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {0};

uint8_t QueryLatestDeviceContext(ComPtr<ID3D11DeviceContextBest>& in, ComPtr<ID3D11DeviceContextBest>& out) {
    static const IID versions[] = {
        __uuidof(ID3D11DeviceContext4),
        __uuidof(ID3D11DeviceContext3),
        __uuidof(ID3D11DeviceContext2),
        __uuidof(ID3D11DeviceContext1),
        __uuidof(ID3D11DeviceContext),
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

CommandBufferD3D11::CommandBufferD3D11(DeviceD3D11& device)
    : m_Device(device)
    , m_DeferredContext(device.GetImmediateContext())
    , m_Version(device.GetImmediateContextVersion()) {
    m_DeferredContext->QueryInterface(IID_PPV_ARGS(&m_Annotation));
}

CommandBufferD3D11::~CommandBufferD3D11() {
    if (m_DeferredContext && m_DeferredContext->GetType() == D3D11_DEVICE_CONTEXT_DEFERRED)
        m_Device.GetExt()->EndUAVOverlap(m_DeferredContext);
}

//================================================================================================================
// CommandBufferHelper
//================================================================================================================

Result CommandBufferD3D11::Create(ID3D11DeviceContext* precreatedContext) {
    // Release inherited interfaces from the immediate context
    m_DeferredContext = nullptr;
    m_Annotation = nullptr;

    // Create deferred context
    ComPtr<ID3D11DeviceContextBest> context = (ID3D11DeviceContextBest*)precreatedContext; // can be immediate
    if (!precreatedContext) {
        HRESULT hr = m_Device->CreateDeferredContext(0, (ID3D11DeviceContext**)&context);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateDeferredContext()");
    }

    m_Version = QueryLatestDeviceContext(context, m_DeferredContext);

    HRESULT hr = m_DeferredContext->QueryInterface(IID_PPV_ARGS(&m_Annotation));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "QueryInterface(ID3DUserDefinedAnnotation)");

    // Skip UAV barriers by default on the deferred context
    if (m_DeferredContext && m_DeferredContext->GetType() == D3D11_DEVICE_CONTEXT_DEFERRED)
        m_Device.GetExt()->BeginUAVOverlap(m_DeferredContext);

    return Result::SUCCESS;
}

void CommandBufferD3D11::Submit() {
    m_Device.GetImmediateContext()->ExecuteCommandList(m_CommandList, FALSE);
    m_CommandList = nullptr;
}

StdAllocator<uint8_t>& CommandBufferD3D11::GetStdAllocator() const {
    return m_Device.GetStdAllocator();
}

//================================================================================================================
// NRI
//================================================================================================================

Result CommandBufferD3D11::Begin(const DescriptorPool* descriptorPool) {
    m_CommandList = nullptr;
    m_Pipeline = nullptr;
    m_IndexBuffer = nullptr;
    m_VertexBuffer = nullptr;

    ResetAttachments();

    // Dynamic state
    m_SamplePositionsState.Reset();
    m_StencilRef = 0;
    m_BlendFactor = {};

    if (descriptorPool)
        SetDescriptorPool(*descriptorPool);

    return Result::SUCCESS;
}

Result CommandBufferD3D11::End() {
    HRESULT hr = m_DeferredContext->FinishCommandList(FALSE, &m_CommandList);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11DeviceContext::FinishCommandList()");

    m_BindingState.UnbindAndReset(m_DeferredContext);

    return Result::SUCCESS;
}

void CommandBufferD3D11::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    m_DeferredContext->RSSetViewports(viewportNum, (const D3D11_VIEWPORT*)viewports);
}

void CommandBufferD3D11::SetScissors(const Rect* rects, uint32_t rectNum) {
    D3D11_RECT* rectsD3D = StackAlloc(D3D11_RECT, rectNum);

    for (uint32_t i = 0; i < rectNum; i++) {
        const Rect& rect = rects[i];
        rectsD3D[i] = {rect.x, rect.y, (LONG)(rect.x + rect.width), (LONG)(rect.y + rect.height)};
    }

    m_DeferredContext->RSSetScissorRects(rectNum, &rectsD3D[0]);
}

void CommandBufferD3D11::SetDepthBounds(float boundsMin, float boundsMax) {
    if (m_DepthBounds[0] != boundsMin || m_DepthBounds[1] != boundsMax) {
        m_Device.GetExt()->SetDepthBounds(m_DeferredContext, boundsMin, boundsMax);

        m_DepthBounds[0] = boundsMin;
        m_DepthBounds[1] = boundsMax;
    }
}

void CommandBufferD3D11::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    MaybeUnused(backRef);

    if (m_Pipeline)
        m_Pipeline->ChangeStencilReference(m_DeferredContext, frontRef);

    m_StencilRef = frontRef;
}

void CommandBufferD3D11::SetSampleLocations(const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    MaybeUnused(sampleNum); // already have this in "m_RasterizerDesc"

    m_SamplePositionsState.Set(locations, locationNum);

    if (m_Pipeline)
        m_Pipeline->ChangeSamplePositions(m_DeferredContext, m_SamplePositionsState);
}

void CommandBufferD3D11::SetBlendConstants(const Color32f& color) {
    if (m_Pipeline)
        m_Pipeline->ChangeBlendConstants(m_DeferredContext, color);

    m_BlendFactor = color;
}

void CommandBufferD3D11::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    if (!clearDescNum)
        return;

    if (!rectNum) {
        for (uint32_t i = 0; i < clearDescNum; i++) {
            const ClearDesc& clearDesc = clearDescs[i];

            if (clearDesc.planes & PlaneBits::COLOR)
                m_DeferredContext->ClearRenderTargetView(m_RenderTargets[clearDesc.colorAttachmentIndex], &clearDesc.value.color.f.x);
            else {
                uint32_t clearFlags = 0;
                if (clearDescs[i].planes & PlaneBits::DEPTH)
                    clearFlags |= D3D11_CLEAR_DEPTH;
                if (clearDescs[i].planes & PlaneBits::STENCIL)
                    clearFlags |= D3D11_CLEAR_STENCIL;

                m_DeferredContext->ClearDepthStencilView(m_DepthStencil, clearFlags, clearDesc.value.depthStencil.depth, clearDesc.value.depthStencil.stencil);
            }
        }
    } else {
        D3D11_RECT* rectsD3D = StackAlloc(D3D11_RECT, rectNum);
        for (uint32_t i = 0; i < rectNum; i++) {
            const Rect& rect = rects[i];
            rectsD3D[i] = {rect.x, rect.y, (LONG)(rect.x + rect.width), (LONG)(rect.y + rect.height)};
        }

        if (m_Version >= 1) {
            // https://learn.microsoft.com/en-us/windows/win32/api/d3d11_1/nf-d3d11_1-id3d11devicecontext1-clearview
            FLOAT color[4] = {};
            for (uint32_t i = 0; i < clearDescNum; i++) {
                const ClearDesc& clearDesc = clearDescs[i];

                if (clearDesc.planes & PlaneBits::COLOR)
                    m_DeferredContext->ClearView(m_RenderTargets[clearDesc.colorAttachmentIndex], &clearDesc.value.color.f.x, rectsD3D, rectNum);
                else if (clearDesc.planes & PlaneBits::DEPTH) {
                    color[0] = clearDesc.value.depthStencil.depth;
                    m_DeferredContext->ClearView(m_DepthStencil, color, rectsD3D, rectNum);
                } else
                    CHECK(false, "Bad or unsupported plane");
            }
        } else
            CHECK(false, "'ClearView' emulation for 11.0 is not implemented!");
    }
}

void CommandBufferD3D11::ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc) {
    const DescriptorD3D11& descriptor = *(const DescriptorD3D11*)clearDesc.storageBuffer;

    Color32ui clearValue = {clearDesc.value, clearDesc.value, clearDesc.value, clearDesc.value};
    m_DeferredContext->ClearUnorderedAccessViewUint(descriptor, &clearValue.x);
}

void CommandBufferD3D11::ClearStorageTexture(const ClearStorageTextureDesc& clearDesc) {
    const DescriptorD3D11& descriptor = *(const DescriptorD3D11*)clearDesc.storageTexture;

    if (descriptor.IsIntegerFormat())
        m_DeferredContext->ClearUnorderedAccessViewUint(descriptor, &clearDesc.value.color.ui.x);
    else
        m_DeferredContext->ClearUnorderedAccessViewFloat(descriptor, &clearDesc.value.color.f.x);
}

void CommandBufferD3D11::BeginRendering(const AttachmentsDesc& attachmentsDesc) {
    // Render targets
    m_RenderTargetNum = attachmentsDesc.colors ? attachmentsDesc.colorNum : 0;

    size_t i = 0;
    for (; i < m_RenderTargetNum; i++) {
        const DescriptorD3D11& descriptor = *(DescriptorD3D11*)attachmentsDesc.colors[i];
        m_RenderTargets[i] = descriptor;
    }
    for (; i < m_RenderTargets.size(); i++)
        m_RenderTargets[i] = nullptr;

    if (attachmentsDesc.depthStencil) {
        const DescriptorD3D11& descriptor = *(DescriptorD3D11*)attachmentsDesc.depthStencil;
        m_DepthStencil = descriptor;
    } else
        m_DepthStencil = nullptr;

    m_DeferredContext->OMSetRenderTargets(m_RenderTargetNum, m_RenderTargets.data(), m_DepthStencil);

    // Shading rate
#if NRI_USE_EXT_LIBS
    if (m_Device.GetExt()->HasNVAPI() && m_Device.GetDesc().shadingRateTier >= 2) {
        ID3D11NvShadingRateResourceView* shadingRateImage = nullptr;
        if (attachmentsDesc.shadingRate) {
            const DescriptorD3D11& descriptor = *(DescriptorD3D11*)attachmentsDesc.shadingRate;
            shadingRateImage = descriptor;

            // Program shading rate lookup table
            if (!m_IsShadingRateLookupTableSet) {
                std::array<NV_D3D11_VIEWPORT_SHADING_RATE_DESC_V1, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> shadingRates = {};
                for (i = 0; i < shadingRates.size(); i++) {
                    shadingRates[i].enableVariablePixelShadingRate = true;

                    for (size_t j = 0; j < GetCountOf(shadingRates[i].shadingRateTable); j++)
                        shadingRates[i].shadingRateTable[j] = NV_PIXEL_X1_PER_RASTER_PIXEL; // be on the safe side, avoid culling

                    shadingRates[i].shadingRateTable[NRI_SHADING_RATE(0, 0)] = NV_PIXEL_X1_PER_RASTER_PIXEL;
                    shadingRates[i].shadingRateTable[NRI_SHADING_RATE(0, 1)] = NV_PIXEL_X1_PER_1X2_RASTER_PIXELS;
                    shadingRates[i].shadingRateTable[NRI_SHADING_RATE(1, 0)] = NV_PIXEL_X1_PER_2X1_RASTER_PIXELS;
                    shadingRates[i].shadingRateTable[NRI_SHADING_RATE(1, 1)] = NV_PIXEL_X1_PER_2X2_RASTER_PIXELS;
                    shadingRates[i].shadingRateTable[NRI_SHADING_RATE(1, 2)] = NV_PIXEL_X1_PER_2X4_RASTER_PIXELS;
                    shadingRates[i].shadingRateTable[NRI_SHADING_RATE(2, 1)] = NV_PIXEL_X1_PER_4X2_RASTER_PIXELS;
                    shadingRates[i].shadingRateTable[NRI_SHADING_RATE(2, 2)] = NV_PIXEL_X1_PER_4X4_RASTER_PIXELS;
                }

                NV_D3D11_VIEWPORTS_SHADING_RATE_DESC shadingRateDesc = {NV_D3D11_VIEWPORTS_SHADING_RATE_DESC_VER};
                shadingRateDesc.numViewports = (uint32_t)shadingRates.size();
                shadingRateDesc.pViewports = shadingRates.data();

                REPORT_ERROR_ON_BAD_STATUS(&m_Device, NvAPI_D3D11_RSSetViewportsPixelShadingRates(m_DeferredContext, &shadingRateDesc));

                m_IsShadingRateLookupTableSet = true;
            }
        }

        REPORT_ERROR_ON_BAD_STATUS(&m_Device, NvAPI_D3D11_RSSetShadingRateResourceView(m_DeferredContext, shadingRateImage));
    }
#endif
}

void CommandBufferD3D11::SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    if (!offsets)
        offsets = s_nullOffsets;

    if (m_VertexBuffer != buffers[0] || m_VertexBufferOffset != offsets[0] || m_VertexBufferBaseSlot != baseSlot || bufferNum > 1) {
        uint8_t* mem = StackAlloc(uint8_t, bufferNum * (sizeof(ID3D11Buffer*) + sizeof(uint32_t) * 2));

        ID3D11Buffer** buf = (ID3D11Buffer**)mem;
        mem += bufferNum * sizeof(ID3D11Buffer*);

        uint32_t* offsetsUint = (uint32_t*)mem;
        mem += bufferNum * sizeof(uint32_t);

        uint32_t* strides = (uint32_t*)mem;

        for (uint32_t i = 0; i < bufferNum; i++) {
            const BufferD3D11& bufferD3D11 = *(BufferD3D11*)buffers[i];
            buf[i] = bufferD3D11;

            strides[i] = m_Pipeline->GetInputAssemblyStride(baseSlot + i);
            offsetsUint[i] = (uint32_t)offsets[i];
        }

        m_DeferredContext->IASetVertexBuffers(baseSlot, bufferNum, buf, strides, offsetsUint);

        m_VertexBuffer = buffers[0];
        m_VertexBufferOffset = offsets[0];
        m_VertexBufferBaseSlot = baseSlot;
    }
}

void CommandBufferD3D11::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    if (m_IndexBuffer != &buffer || m_IndexBufferOffset != offset || m_IndexType != indexType) {
        const BufferD3D11& bufferD3D11 = (BufferD3D11&)buffer;
        const DXGI_FORMAT format = indexType == IndexType::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

        m_DeferredContext->IASetIndexBuffer(bufferD3D11, format, (uint32_t)offset);

        m_IndexBuffer = &buffer;
        m_IndexBufferOffset = offset;
        m_IndexType = indexType;
    }
}

void CommandBufferD3D11::SetPipelineLayout(const PipelineLayout& pipelineLayout) {
    PipelineLayoutD3D11* pipelineLayoutD3D11 = (PipelineLayoutD3D11*)&pipelineLayout;
    pipelineLayoutD3D11->Bind(m_DeferredContext);

    m_PipelineLayout = pipelineLayoutD3D11;
}

void CommandBufferD3D11::SetPipeline(const Pipeline& pipeline) {
    PipelineD3D11* pipelineD3D11 = (PipelineD3D11*)&pipeline;
    pipelineD3D11->Bind(m_DeferredContext, m_Pipeline, m_StencilRef, m_BlendFactor, m_SamplePositionsState);

    m_Pipeline = pipelineD3D11;
}

void CommandBufferD3D11::SetDescriptorPool(const DescriptorPool& descriptorPool) {
    MaybeUnused(descriptorPool);
}

void CommandBufferD3D11::SetDescriptorSet(uint32_t setIndex, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    const DescriptorSetD3D11& descriptorSetImpl = (DescriptorSetD3D11&)descriptorSet;
    m_PipelineLayout->BindDescriptorSet(m_BindingState, m_DeferredContext, setIndex, descriptorSetImpl, dynamicConstantBufferOffsets);
}

void CommandBufferD3D11::SetConstants(uint32_t pushConstantIndex, const void* data, uint32_t size) {
    m_PipelineLayout->SetConstants(m_DeferredContext, pushConstantIndex, (const Vec4*)data, size);
}

void CommandBufferD3D11::Draw(const DrawDesc& drawDesc) {
    m_DeferredContext->DrawInstanced(drawDesc.vertexNum, drawDesc.instanceNum, drawDesc.baseVertex, drawDesc.baseInstance);
}

void CommandBufferD3D11::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    m_DeferredContext->DrawIndexedInstanced(
        drawIndexedDesc.indexNum, drawIndexedDesc.instanceNum, drawIndexedDesc.baseIndex, drawIndexedDesc.baseVertex, drawIndexedDesc.baseInstance);
}

void CommandBufferD3D11::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    MaybeUnused(countBuffer, countBufferOffset);

    if (countBuffer && m_Device.GetDesc().isDrawIndirectCountSupported)
        m_Device.GetExt()->DrawIndirect(m_DeferredContext, (BufferD3D11&)buffer, offset, drawNum, stride, *(BufferD3D11*)countBuffer, (uint32_t)countBufferOffset);
    else
        m_Device.GetExt()->DrawIndirect(m_DeferredContext, (BufferD3D11&)buffer, offset, drawNum, stride, nullptr, 0);
}

void CommandBufferD3D11::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    MaybeUnused(countBuffer, countBufferOffset);

    if (countBuffer && m_Device.GetDesc().isDrawIndirectCountSupported)
        m_Device.GetExt()->DrawIndexedIndirect(m_DeferredContext, (BufferD3D11&)buffer, offset, drawNum, stride, *(BufferD3D11*)countBuffer, (uint32_t)countBufferOffset);
    else
        m_Device.GetExt()->DrawIndexedIndirect(m_DeferredContext, (BufferD3D11&)buffer, offset, drawNum, stride, nullptr, 0);
}

void CommandBufferD3D11::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    BufferD3D11& dst = (BufferD3D11&)dstBuffer;
    BufferD3D11& src = (BufferD3D11&)srcBuffer;

    if (size == WHOLE_SIZE)
        size = src.GetDesc().size;

    bool isEntireResource = (srcOffset == 0 && dstOffset == 0);
    isEntireResource &= src.GetDesc().size == size;
    isEntireResource &= dst.GetDesc().size == size;

    if (isEntireResource)
        m_DeferredContext->CopyResource(dst, src);
    else {
        D3D11_BOX box = {};
        box.left = uint32_t(srcOffset);
        box.right = uint32_t(srcOffset + size);
        box.bottom = 1;
        box.back = 1;

        m_DeferredContext->CopySubresourceRegion(dst, 0, (uint32_t)dstOffset, 0, 0, src, 0, &box);
    }
}

void CommandBufferD3D11::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    TextureD3D11& dst = (TextureD3D11&)dstTexture;
    TextureD3D11& src = (TextureD3D11&)srcTexture;

    if ((!dstRegionDesc || !srcRegionDesc) || (dstRegionDesc->mipOffset == NULL_TEXTURE_REGION_DESC && srcRegionDesc->mipOffset == NULL_TEXTURE_REGION_DESC))
        m_DeferredContext->CopyResource(dst, src);
    else {
        D3D11_BOX srcBox = {};
        srcBox.left = srcRegionDesc->x;
        srcBox.top = srcRegionDesc->y;
        srcBox.front = srcRegionDesc->z;
        srcBox.right = srcRegionDesc->width == WHOLE_SIZE ? src.GetSize(0, srcRegionDesc->mipOffset) : srcRegionDesc->width;
        srcBox.bottom = srcRegionDesc->height == WHOLE_SIZE ? src.GetSize(1, srcRegionDesc->mipOffset) : srcRegionDesc->height;
        srcBox.back = srcRegionDesc->depth == WHOLE_SIZE ? src.GetSize(2, srcRegionDesc->mipOffset) : srcRegionDesc->depth;
        srcBox.right += srcBox.left;
        srcBox.bottom += srcBox.top;
        srcBox.back += srcBox.front;

        uint32_t dstSubresource = dst.GetSubresourceIndex(*dstRegionDesc);
        uint32_t srcSubresource = src.GetSubresourceIndex(*srcRegionDesc);

        m_DeferredContext->CopySubresourceRegion(dst, dstSubresource, dstRegionDesc->x, dstRegionDesc->y, dstRegionDesc->z, src, srcSubresource, &srcBox);
    }
}

void CommandBufferD3D11::UploadBufferToTexture(
    Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {
    BufferD3D11& src = (BufferD3D11&)srcBuffer;
    TextureD3D11& dst = (TextureD3D11&)dstTexture;

    D3D11_BOX dstBox = {};
    dstBox.left = dstRegionDesc.x;
    dstBox.top = dstRegionDesc.y;
    dstBox.front = dstRegionDesc.z;
    dstBox.right = dstRegionDesc.width == WHOLE_SIZE ? dst.GetSize(0, dstRegionDesc.mipOffset) : dstRegionDesc.width;
    dstBox.bottom = dstRegionDesc.height == WHOLE_SIZE ? dst.GetSize(1, dstRegionDesc.mipOffset) : dstRegionDesc.height;
    dstBox.back = dstRegionDesc.depth == WHOLE_SIZE ? dst.GetSize(2, dstRegionDesc.mipOffset) : dstRegionDesc.depth;
    dstBox.right += dstBox.left;
    dstBox.bottom += dstBox.top;
    dstBox.back += dstBox.front;

    uint32_t dstSubresource = dst.GetSubresourceIndex(dstRegionDesc);

    uint8_t* data = (uint8_t*)src.Map(srcDataLayoutDesc.offset);
    m_DeferredContext->UpdateSubresource(dst, dstSubresource, &dstBox, data, srcDataLayoutDesc.rowPitch, srcDataLayoutDesc.slicePitch);
    src.Unmap();
}

void CommandBufferD3D11::ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {
    CHECK(dstDataLayoutDesc.offset == 0, "D3D11 implementation currently supports copying a texture region to a buffer only with offset = 0!");

    BufferD3D11& dst = (BufferD3D11&)dstBuffer;
    TextureD3D11& src = (TextureD3D11&)srcTexture;

    TextureD3D11& dstTemp = dst.RecreateReadbackTexture(src, srcRegionDesc, dstDataLayoutDesc);

    TextureRegionDesc dstRegionDesc = {};
    dstRegionDesc.mipOffset = srcRegionDesc.mipOffset;
    dstRegionDesc.layerOffset = srcRegionDesc.layerOffset;
    dstRegionDesc.width = srcRegionDesc.width == WHOLE_SIZE ? src.GetSize(0, srcRegionDesc.mipOffset) : srcRegionDesc.width;
    dstRegionDesc.height = srcRegionDesc.height == WHOLE_SIZE ? src.GetSize(1, srcRegionDesc.mipOffset) : srcRegionDesc.height;
    dstRegionDesc.depth = srcRegionDesc.depth == WHOLE_SIZE ? src.GetSize(2, srcRegionDesc.mipOffset) : srcRegionDesc.depth;

    CopyTexture((Texture&)dstTemp, &dstRegionDesc, srcTexture, &srcRegionDesc);
}

void CommandBufferD3D11::Dispatch(const DispatchDesc& dispatchDesc) {
    m_DeferredContext->Dispatch(dispatchDesc.x, dispatchDesc.y, dispatchDesc.z);
}

void CommandBufferD3D11::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    m_DeferredContext->DispatchIndirect((BufferD3D11&)buffer, (uint32_t)offset);
}

void CommandBufferD3D11::Barrier(const BarrierGroupDesc& barrierGroupDesc) {
    MaybeUnused(barrierGroupDesc);
#if NRI_USE_EXT_LIBS
    if (barrierGroupDesc.textureNum == 0 && barrierGroupDesc.bufferNum == 0)
        return;

    uint32_t flags = 0;

    for (uint32_t i = 0; i < barrierGroupDesc.globalNum; i++) {
        const GlobalBarrierDesc& barrier = barrierGroupDesc.globals[i];
        if ((barrier.before.access & AccessBits::SHADER_RESOURCE_STORAGE) && (barrier.after.access & AccessBits::SHADER_RESOURCE_STORAGE)) {
            bool isGraphics = barrier.before.stages == StageBits::ALL || (barrier.before.stages & (StageBits::DRAW));
            if (isGraphics)
                flags |= NVAPI_D3D_BEGIN_UAV_OVERLAP_GFX_WFI;

            bool isCompute = barrier.before.stages == StageBits::ALL || (barrier.before.stages & StageBits::COMPUTE_SHADER);
            if (isCompute)
                flags |= NVAPI_D3D_BEGIN_UAV_OVERLAP_COMP_WFI;
        }
    }

    for (uint32_t i = 0; i < barrierGroupDesc.bufferNum; i++) {
        const BufferBarrierDesc& barrier = barrierGroupDesc.buffers[i];
        if ((barrier.before.access & AccessBits::SHADER_RESOURCE_STORAGE) && (barrier.after.access & AccessBits::SHADER_RESOURCE_STORAGE)) {
            bool isGraphics = barrier.before.stages == StageBits::ALL || (barrier.before.stages & (StageBits::DRAW));
            if (isGraphics)
                flags |= NVAPI_D3D_BEGIN_UAV_OVERLAP_GFX_WFI;

            bool isCompute = barrier.before.stages == StageBits::ALL || (barrier.before.stages & StageBits::COMPUTE_SHADER);
            if (isCompute)
                flags |= NVAPI_D3D_BEGIN_UAV_OVERLAP_COMP_WFI;
        }
    }

    for (uint32_t i = 0; i < barrierGroupDesc.textureNum; i++) {
        const TextureBarrierDesc& barrier = barrierGroupDesc.textures[i];
        if ((barrier.before.access & AccessBits::SHADER_RESOURCE_STORAGE) && (barrier.after.access & AccessBits::SHADER_RESOURCE_STORAGE)) {
            bool isGraphics = barrier.before.stages == StageBits::ALL || (barrier.before.stages & (StageBits::DRAW));
            if (isGraphics)
                flags |= NVAPI_D3D_BEGIN_UAV_OVERLAP_GFX_WFI;

            bool isCompute = barrier.before.stages == StageBits::ALL || (barrier.before.stages & StageBits::COMPUTE_SHADER);
            if (isCompute)
                flags |= NVAPI_D3D_BEGIN_UAV_OVERLAP_COMP_WFI;
        }
    }

    if (flags)
        m_Device.GetExt()->WaitForDrain(m_DeferredContext, flags);
#endif
}

void CommandBufferD3D11::BeginQuery(const QueryPool& queryPool, uint32_t offset) {
    ((QueryPoolD3D11&)queryPool).BeginQuery(m_DeferredContext, offset);
}

void CommandBufferD3D11::EndQuery(const QueryPool& queryPool, uint32_t offset) {
    ((QueryPoolD3D11&)queryPool).EndQuery(m_DeferredContext, offset);
}

void CommandBufferD3D11::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    ((BufferD3D11&)dstBuffer).AssignQueryPoolRange((QueryPoolD3D11*)&queryPool, offset, num, dstOffset);
}

void CommandBufferD3D11::BeginAnnotation(const char* name) {
    size_t len = strlen(name) + 1;
    wchar_t* s = StackAlloc(wchar_t, len);
    ConvertCharToWchar(name, s, len);

    m_Annotation->BeginEvent(s);
}

void CommandBufferD3D11::EndAnnotation() {
    m_Annotation->EndEvent();
}

#include "CommandBufferD3D11.hpp"
