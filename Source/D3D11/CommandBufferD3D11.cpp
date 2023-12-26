/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "SharedD3D11.h"
#include "CommandBufferD3D11.h"
#include "BufferD3D11.h"
#include "DescriptorD3D11.h"
#include "DescriptorSetD3D11.h"
#include "QueryPoolD3D11.h"
#include "PipelineLayoutD3D11.h"
#include "PipelineD3D11.h"
#include "TextureD3D11.h"

using namespace nri;

static constexpr uint64_t s_nullOffsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {0};

CommandBufferD3D11::CommandBufferD3D11(DeviceD3D11& device) :
    m_Device(device),
    m_DeferredContext(device.GetImmediateContext())
{
    m_DeferredContext->QueryInterface(IID_PPV_ARGS(&m_Annotation));
    m_DeferredContext.ext->BeginUAVOverlap(m_DeferredContext);
}

CommandBufferD3D11::~CommandBufferD3D11()
{
    m_DeferredContext.ext->EndUAVOverlap(m_DeferredContext);
}

//================================================================================================================
// CommandBufferHelper
//================================================================================================================

Result CommandBufferD3D11::Create(ID3D11DeviceContext* precreatedContext)
{
    HRESULT hr;
    ComPtr<ID3D11DeviceContext> context = precreatedContext;

    if (!precreatedContext)
    {
        hr = m_Device.GetDevice()->CreateDeferredContext(0, &context);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateDeferredContext()");
    }

    // Release inherited interfaces from the immediate context
    m_DeferredContext.ptr = nullptr;
    m_Annotation = nullptr;

    hr = context->QueryInterface(__uuidof(ID3D11DeviceContext4), (void**)&m_DeferredContext.ptr);
    m_DeferredContext.version = 4;
    m_DeferredContext.ext = m_Device.GetDevice().ext;
    if (FAILED(hr))
    {
        REPORT_WARNING(&m_Device, "QueryInterface(ID3D11DeviceContext4) - FAILED!");
        hr = context->QueryInterface(__uuidof(ID3D11DeviceContext3), (void**)&m_DeferredContext.ptr);
        m_DeferredContext.version = 3;
        if (FAILED(hr))
        {
            REPORT_WARNING(&m_Device, "QueryInterface(ID3D11DeviceContext3) - FAILED!");
            hr = context->QueryInterface(__uuidof(ID3D11DeviceContext2), (void**)&m_DeferredContext.ptr);
            m_DeferredContext.version = 2;
            if (FAILED(hr))
            {
                REPORT_WARNING(&m_Device, "QueryInterface(ID3D11DeviceContext2) - FAILED!");
                hr = context->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&m_DeferredContext.ptr);
                m_DeferredContext.version = 1;
                if (FAILED(hr))
                {
                    REPORT_WARNING(&m_Device, "QueryInterface(ID3D11DeviceContext1) - FAILED!");
                    m_DeferredContext.ptr = (ID3D11DeviceContext4*)context.GetInterface();
                    m_DeferredContext.version = 0;
                }
            }
        }
    }

    hr = m_DeferredContext->QueryInterface(IID_PPV_ARGS(&m_Annotation));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "QueryInterface(ID3DUserDefinedAnnotation)");

    m_DeferredContext.ext->BeginUAVOverlap(m_DeferredContext);

    return Result::SUCCESS;
}

void CommandBufferD3D11::Submit()
{
    m_Device.GetImmediateContext()->ExecuteCommandList(m_CommandList, FALSE);
    m_CommandList = nullptr;
}

ID3D11DeviceContext* CommandBufferD3D11::GetNativeObject() const
{
    return m_DeferredContext.ptr;
}

StdAllocator<uint8_t>& CommandBufferD3D11::GetStdAllocator() const
{
    return m_Device.GetStdAllocator();
}

//================================================================================================================
// NRI
//================================================================================================================

Result CommandBufferD3D11::Begin(const DescriptorPool* descriptorPool)
{
    m_SamplePositionsState.Reset();
    m_CommandList = nullptr;
    m_Pipeline = nullptr;
    m_IndexBuffer = nullptr;
    m_VertexBuffer = nullptr;
    m_StencilRef = 0;

    SetDepthBounds(0.0f, 1.0f);

    if (descriptorPool)
        SetDescriptorPool(*descriptorPool);

    return Result::SUCCESS;
}

Result CommandBufferD3D11::End()
{
    HRESULT hr = m_DeferredContext->FinishCommandList(FALSE, &m_CommandList);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11DeviceContext::FinishCommandList()");

    m_BindingState.UnbindAndReset(m_DeferredContext);

    return Result::SUCCESS;
}

void CommandBufferD3D11::SetViewports(const Viewport* viewports, uint32_t viewportNum)
{
    m_DeferredContext->RSSetViewports(viewportNum, (const D3D11_VIEWPORT*)viewports);
}

void CommandBufferD3D11::SetScissors(const Rect* rects, uint32_t rectNum)
{
    D3D11_RECT* rectsD3D = STACK_ALLOC(D3D11_RECT, rectNum);

    for (uint32_t i = 0; i < rectNum; i++)
    {
        const Rect& rect = rects[i];
        rectsD3D[i] = { rect.x, rect.y, (LONG)(rect.x + rect.width), (LONG)(rect.y + rect.height) };
    }

    if (!m_Pipeline || !m_Pipeline->IsRasterizerDiscarded())
        m_DeferredContext->RSSetScissorRects(rectNum, &rectsD3D[0]);
}

void CommandBufferD3D11::SetDepthBounds(float boundsMin, float boundsMax)
{
    if (m_DepthBounds[0] != boundsMin || m_DepthBounds[1] != boundsMax)
    {
        m_DeferredContext.ext->SetDepthBounds(m_DeferredContext, boundsMin, boundsMax);

        m_DepthBounds[0] = boundsMin;
        m_DepthBounds[1] = boundsMax;
    }
}

void CommandBufferD3D11::SetStencilReference(uint8_t reference)
{
    m_StencilRef = reference;

    if (m_Pipeline)
        m_Pipeline->ChangeStencilReference(m_DeferredContext, m_StencilRef, DynamicState::BIND_AND_SET);
}

void CommandBufferD3D11::SetSamplePositions(const SamplePosition* positions, uint32_t positionNum)
{
    m_SamplePositionsState.Set(positions, positionNum);

    if (m_Pipeline)
        m_Pipeline->ChangeSamplePositions(m_DeferredContext, m_SamplePositionsState, DynamicState::BIND_AND_SET);
}

void CommandBufferD3D11::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum)
{
    if (!rects || !rectNum)
    {
        for (uint32_t i = 0; i < clearDescNum; i++)
        {
            const ClearDesc& clearDesc = clearDescs[i];

            switch (clearDesc.attachmentContentType)
            {
            case AttachmentContentType::COLOR:
                m_DeferredContext->ClearRenderTargetView(m_RenderTargets[clearDesc.colorAttachmentIndex], &clearDesc.value.color32f.x);
                break;
            case AttachmentContentType::DEPTH:
                m_DeferredContext->ClearDepthStencilView(m_DepthStencil, D3D11_CLEAR_DEPTH, clearDesc.value.depthStencil.depth, 0);
                break;
            case AttachmentContentType::STENCIL:
                m_DeferredContext->ClearDepthStencilView(m_DepthStencil, D3D11_CLEAR_STENCIL, 0.0f, clearDesc.value.depthStencil.stencil);
                break;
            case AttachmentContentType::DEPTH_STENCIL:
                m_DeferredContext->ClearDepthStencilView(m_DepthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearDesc.value.depthStencil.depth, clearDesc.value.depthStencil.stencil);
                break;
            }
        }
    }
    else
    {
        D3D11_RECT* rectsD3D = STACK_ALLOC(D3D11_RECT, rectNum);
        for (uint32_t i = 0; i < rectNum; i++)
        {
            const Rect& rect = rects[i];
            rectsD3D[i] = { rect.x, rect.y, (LONG)(rect.x + rect.width), (LONG)(rect.y + rect.height) };
        }

        if (m_DeferredContext.version >= 1)
        {
            // https://learn.microsoft.com/en-us/windows/win32/api/d3d11_1/nf-d3d11_1-id3d11devicecontext1-clearview

            FLOAT color[4] = {};
            for (uint32_t i = 0; i < clearDescNum; i++)
            {
                const ClearDesc& clearDesc = clearDescs[i];
                switch (clearDesc.attachmentContentType)
                {
                case AttachmentContentType::COLOR:
                    m_DeferredContext->ClearView(m_RenderTargets[clearDesc.colorAttachmentIndex], &clearDesc.value.color32f.x, rectsD3D, rectNum);
                    break;
                case AttachmentContentType::DEPTH: case AttachmentContentType::DEPTH_STENCIL:
                    color[0] = clearDesc.value.depthStencil.depth;
                    m_DeferredContext->ClearView(m_DepthStencil, color, rectsD3D, rectNum);
                    break;
                case AttachmentContentType::STENCIL:
                    color[0] = clearDesc.value.depthStencil.stencil; // TODO: should work, no?
                    m_DeferredContext->ClearView(m_DepthStencil, color, rectsD3D, rectNum);
                    break;
                }
            }
        }
        else
            CHECK(&m_Device, false, "'ClearView' emulation for 11.0 is not implemented!");
    }
}

void CommandBufferD3D11::ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc)
{
    const DescriptorD3D11& descriptor = *(const DescriptorD3D11*)clearDesc.storageBuffer;

    Color32ui clearValue = {clearDesc.value, clearDesc.value, clearDesc.value, clearDesc.value};
    m_DeferredContext->ClearUnorderedAccessViewUint(descriptor, &clearValue.x);
}

void CommandBufferD3D11::ClearStorageTexture(const ClearStorageTextureDesc& clearDesc)
{
    const DescriptorD3D11& descriptor = *(const DescriptorD3D11*)clearDesc.storageTexture;

    if (descriptor.IsIntegerFormat())
        m_DeferredContext->ClearUnorderedAccessViewUint(descriptor, &clearDesc.value.color32ui.x);
    else
        m_DeferredContext->ClearUnorderedAccessViewFloat(descriptor, &clearDesc.value.color32f.x);
}

void CommandBufferD3D11::BeginRendering(const AttachmentsDesc& attachmentsDesc)
{
    m_RenderTargetNum = attachmentsDesc.colors ? attachmentsDesc.colorNum : 0;
    
    uint32_t i = 0;
    for (; i < m_RenderTargetNum; i++)
    {
        const DescriptorD3D11& descriptor = *(DescriptorD3D11*)attachmentsDesc.colors[i];
        m_RenderTargets[i] = descriptor;
    }
    for (; i < (uint32_t)m_RenderTargets.size(); i++)
        m_RenderTargets[i] = nullptr;

    if (attachmentsDesc.depthStencil)
    {
        const DescriptorD3D11& descriptor = *(DescriptorD3D11*)attachmentsDesc.depthStencil;
        m_DepthStencil = descriptor;
    }
    else
        m_DepthStencil = nullptr;

    m_DeferredContext->OMSetRenderTargets(m_RenderTargetNum, m_RenderTargets.data(), m_DepthStencil);
}

void CommandBufferD3D11::SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets)
{
    if (!offsets)
        offsets = s_nullOffsets;

    if ( m_VertexBuffer != buffers[0] || m_VertexBufferOffset != offsets[0] || m_VertexBufferBaseSlot != baseSlot || bufferNum > 1)
    {
        uint8_t* mem = STACK_ALLOC( uint8_t, bufferNum * (sizeof(ID3D11Buffer*) + sizeof(uint32_t) * 2) );

        ID3D11Buffer** buf = (ID3D11Buffer**)mem;
        mem += bufferNum * sizeof(ID3D11Buffer*);

        uint32_t* offsetsUint = (uint32_t*)mem;
        mem += bufferNum * sizeof(uint32_t);

        uint32_t* strides = (uint32_t*)mem;

        for (uint32_t i = 0; i < bufferNum; i++)
        {
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

void CommandBufferD3D11::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType)
{
    if (m_IndexBuffer != &buffer || m_IndexBufferOffset != offset || m_IndexType != indexType)
    {
        const BufferD3D11& bufferD3D11 = (BufferD3D11&)buffer;
        const DXGI_FORMAT format = indexType == IndexType::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

        m_DeferredContext->IASetIndexBuffer(bufferD3D11, format, (uint32_t)offset);

        m_IndexBuffer = &buffer;
        m_IndexBufferOffset = offset;
        m_IndexType = indexType;
    }
}

void CommandBufferD3D11::SetPipelineLayout(const PipelineLayout& pipelineLayout)
{
    PipelineLayoutD3D11* pipelineLayoutD3D11 = (PipelineLayoutD3D11*)&pipelineLayout;
    pipelineLayoutD3D11->Bind(m_DeferredContext);

    m_PipelineLayout = pipelineLayoutD3D11;
}

void CommandBufferD3D11::SetPipeline(const Pipeline& pipeline)
{
    PipelineD3D11* pipelineD3D11 = (PipelineD3D11*)&pipeline;
    pipelineD3D11->ChangeSamplePositions(m_DeferredContext, m_SamplePositionsState, DynamicState::SET_ONLY);
    pipelineD3D11->ChangeStencilReference(m_DeferredContext, m_StencilRef, DynamicState::SET_ONLY);
    pipelineD3D11->Bind(m_DeferredContext, m_Pipeline);

    m_Pipeline = pipelineD3D11;
}

void CommandBufferD3D11::SetDescriptorPool(const DescriptorPool& descriptorPool)
{
    MaybeUnused(descriptorPool);
}

void CommandBufferD3D11::SetDescriptorSet(uint32_t setIndexInPipelineLayout, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets)
{
    const DescriptorSetD3D11& descriptorSetImpl = (DescriptorSetD3D11&)descriptorSet;
    m_PipelineLayout->BindDescriptorSet(m_BindingState, m_DeferredContext, setIndexInPipelineLayout, descriptorSetImpl, dynamicConstantBufferOffsets);
}

void CommandBufferD3D11::SetConstants(uint32_t pushConstantIndex, const void* data, uint32_t size)
{
    m_PipelineLayout->SetConstants(m_DeferredContext, pushConstantIndex, (const Vec4*)data, size);
}

void CommandBufferD3D11::Draw(uint32_t vertexNum, uint32_t instanceNum, uint32_t baseVertex, uint32_t baseInstance)
{
    m_DeferredContext->DrawInstanced(vertexNum, instanceNum, baseVertex, baseInstance);
}

void CommandBufferD3D11::DrawIndexed(uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance)
{
    m_DeferredContext->DrawIndexedInstanced(indexNum, instanceNum, baseIndex, baseVertex, baseInstance);
}

void CommandBufferD3D11::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride)
{
    m_DeferredContext.ext->MultiDrawIndirect(m_DeferredContext, (BufferD3D11&)buffer, offset, drawNum, stride);
}

void CommandBufferD3D11::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride)
{
    m_DeferredContext.ext->MultiDrawIndexedIndirect(m_DeferredContext, (BufferD3D11&)buffer, offset, drawNum, stride);
}

void CommandBufferD3D11::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size)
{
    BufferD3D11& dst = (BufferD3D11&)dstBuffer;
    BufferD3D11& src = (BufferD3D11&)srcBuffer;

    if (size == WHOLE_SIZE)
        size = src.GetDesc().size;

    bool isEntireResource = (srcOffset == 0 && dstOffset == 0);
    isEntireResource &= src.GetDesc().size == size;
    isEntireResource &= dst.GetDesc().size == size;

    if (isEntireResource)
        m_DeferredContext->CopyResource(dst, src);
    else
    {
        D3D11_BOX box = {};
        box.left = uint32_t(srcOffset);
        box.right = uint32_t(srcOffset + size);
        box.bottom = 1;
        box.back = 1;

        m_DeferredContext->CopySubresourceRegion(dst, 0, (uint32_t)dstOffset, 0, 0, src, 0, &box);
    }
}

void CommandBufferD3D11::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc)
{
    TextureD3D11& dst = (TextureD3D11&)dstTexture;
    TextureD3D11& src = (TextureD3D11&)srcTexture;

    if ( (!dstRegionDesc || !srcRegionDesc) || (dstRegionDesc->mipOffset == NULL_TEXTURE_REGION_DESC && srcRegionDesc->mipOffset == NULL_TEXTURE_REGION_DESC) )
        m_DeferredContext->CopyResource(dst, src);
    else
    {
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

void CommandBufferD3D11::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc)
{
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

    uint8_t* data = (uint8_t*)src.Map(MapType::READ, srcDataLayoutDesc.offset);
    m_DeferredContext->UpdateSubresource(dst, dstSubresource, &dstBox, data, srcDataLayoutDesc.rowPitch, srcDataLayoutDesc.slicePitch);
    src.Unmap();
}

void CommandBufferD3D11::ReadbackTextureToBuffer(Buffer& dstBuffer, TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc)
{
    CHECK(&m_Device, dstDataLayoutDesc.offset == 0, "D3D11 implementation currently supports copying a texture region to a buffer only with offset = 0!");

    BufferD3D11& dst = (BufferD3D11&)dstBuffer;
    TextureD3D11& src = (TextureD3D11&)srcTexture;

    TextureD3D11& dstTemp = dst.RecreateReadbackTexture(src, srcRegionDesc, dstDataLayoutDesc);

    TextureRegionDesc dstRegionDesc = {};
    dstRegionDesc.mipOffset = srcRegionDesc.mipOffset;
    dstRegionDesc.arrayOffset = srcRegionDesc.arrayOffset;
    dstRegionDesc.width = srcRegionDesc.width == WHOLE_SIZE ? src.GetSize(0, srcRegionDesc.mipOffset) : srcRegionDesc.width;
    dstRegionDesc.height = srcRegionDesc.height == WHOLE_SIZE ? src.GetSize(1, srcRegionDesc.mipOffset) : srcRegionDesc.height;
    dstRegionDesc.depth = srcRegionDesc.depth == WHOLE_SIZE ? src.GetSize(2, srcRegionDesc.mipOffset) : srcRegionDesc.depth;

    CopyTexture((Texture&)dstTemp, &dstRegionDesc, srcTexture, &srcRegionDesc);
}

void CommandBufferD3D11::Dispatch(uint32_t x, uint32_t y, uint32_t z)
{
    m_DeferredContext->Dispatch(x, y, z);
}

void CommandBufferD3D11::DispatchIndirect(const Buffer& buffer, uint64_t offset)
{
    m_DeferredContext->DispatchIndirect((BufferD3D11&)buffer, (uint32_t)offset);
}

void CommandBufferD3D11::PipelineBarrier(const TransitionBarrierDesc* transitionBarriers, const AliasingBarrierDesc* aliasingBarriers, BarrierDependency dependency)
{
    MaybeUnused(aliasingBarriers);

    constexpr AccessBits STORAGE_MASK = AccessBits::SHADER_RESOURCE_STORAGE;
    constexpr BarrierDependency NO_WFI = (BarrierDependency)(-1);

    if (!transitionBarriers || (transitionBarriers->textureNum == 0 && transitionBarriers->bufferNum == 0))
        return;

    BarrierDependency result = NO_WFI;

    for (uint32_t i = 0; i < transitionBarriers->textureNum; i++)
    {
        const TextureTransitionBarrierDesc& textureDesc = transitionBarriers->textures[i];

        if ((textureDesc.prevState.acessBits & STORAGE_MASK) && (textureDesc.nextState.acessBits & STORAGE_MASK))
            result = dependency;
    }

    for (uint32_t i = 0; i < transitionBarriers->bufferNum && result == NO_WFI; i++)
    {
        const BufferTransitionBarrierDesc& bufferDesc = transitionBarriers->buffers[i];

        if ((bufferDesc.prevAccess & STORAGE_MASK) && (bufferDesc.nextAccess & STORAGE_MASK))
            result = dependency;
    }

    if (result != NO_WFI)
        m_DeferredContext.ext->WaitForDrain(m_DeferredContext, result);
}

void CommandBufferD3D11::BeginQuery(const QueryPool& queryPool, uint32_t offset)
{
    ((QueryPoolD3D11&)queryPool).BeginQuery(m_DeferredContext, offset);
}

void CommandBufferD3D11::EndQuery(const QueryPool& queryPool, uint32_t offset)
{
    ((QueryPoolD3D11&)queryPool).EndQuery(m_DeferredContext, offset);
}

void CommandBufferD3D11::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset)
{
    ((BufferD3D11&)dstBuffer).AssignQueryPoolRange((QueryPoolD3D11*)&queryPool, offset, num, dstOffset);
}

void CommandBufferD3D11::BeginAnnotation(const char* name)
{
    size_t len = strlen(name) + 1;
    wchar_t* s = STACK_ALLOC(wchar_t, len);
    ConvertCharToWchar(name, s, len);

    m_Annotation->BeginEvent(s);
}

void CommandBufferD3D11::EndAnnotation()
{
    m_Annotation->EndEvent();
}

#include "CommandBufferD3D11.hpp"
