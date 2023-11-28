/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "SharedD3D12.h"
#include "TextureD3D12.h"
#include "MemoryD3D12.h"

using namespace nri;

static D3D12_RESOURCE_DESC GetResourceDesc(const TextureDesc& textureDesc)
{
    uint16_t blockWidth = (uint16_t)GetFormatProps(textureDesc.format).blockWidth;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = GetResourceDimension(textureDesc.type);
    desc.Alignment = textureDesc.sampleNum > 1 ? 0 : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width = Align(textureDesc.width, blockWidth);
    desc.Height = Align(textureDesc.height, blockWidth);
    desc.DepthOrArraySize = textureDesc.type == TextureType::TEXTURE_3D ? textureDesc.depth : textureDesc.arraySize;
    desc.MipLevels = textureDesc.mipNum;
    desc.Format = GetDxgiFormat(textureDesc.format).typeless;
    desc.SampleDesc.Count = textureDesc.sampleNum;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = GetTextureFlags(textureDesc.usageMask);

    return desc;
}

Result TextureD3D12::Create(const TextureDesc& textureDesc)
{
    m_Desc = textureDesc;

    return Result::SUCCESS;
}

Result TextureD3D12::Create(const TextureD3D12Desc& textureDesc)
{
    if (!GetTextureDesc(textureDesc, m_Desc))
        return Result::INVALID_ARGUMENT;

    m_Texture = textureDesc.d3d12Resource;

    return Result::SUCCESS;
}

Result TextureD3D12::BindMemory(const MemoryD3D12* memory, uint64_t offset)
{
    D3D12_RESOURCE_DESC desc = GetResourceDesc(m_Desc);
    const D3D12_HEAP_DESC& heapDesc = memory->GetHeapDesc();
    D3D12_CLEAR_VALUE clearValue = { GetDxgiFormat(m_Desc.format).typed };
    bool isRenderableSurface = desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    if (memory->RequiresDedicatedAllocation())
    {
        HRESULT hr = ((ID3D12Device*)m_Device)->CreateCommittedResource(
            &heapDesc.Properties,
            D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
            &desc,
            D3D12_RESOURCE_STATE_COMMON,
            isRenderableSurface ? &clearValue : nullptr,
            IID_PPV_ARGS(&m_Texture)
        );

        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommittedResource()");
    }
    else
    {
        HRESULT hr = ((ID3D12Device*)m_Device)->CreatePlacedResource(
            *memory,
            offset,
            &desc,
            D3D12_RESOURCE_STATE_COMMON,
            isRenderableSurface ? &clearValue : nullptr,
            IID_PPV_ARGS(&m_Texture)
        );

        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreatePlacedResource()");
    }

    return Result::SUCCESS;
}

Dim_t TextureD3D12::GetSize(Dim_t dimensionIndex, Mip_t mip) const
{
    assert(dimensionIndex < 3);

    Dim_t dim;
    if (dimensionIndex == 0)
        dim = m_Desc.width;
    else if (dimensionIndex == 1)
        dim = m_Desc.height;
    else
        dim = m_Desc.depth;

    dim = (Dim_t)std::max(dim >> mip, 1);
    dim = Align(dim, dimensionIndex < 2 ? GetFormatProps(m_Desc.format).blockWidth : 1);

    return dim;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void TextureD3D12::GetMemoryInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const
{
    D3D12_RESOURCE_DESC desc = GetResourceDesc(m_Desc);
    m_Device.GetMemoryInfo(memoryLocation, desc, memoryDesc);
}

#include "TextureD3D12.hpp"
