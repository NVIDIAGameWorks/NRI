/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "SharedD3D11.h"
#include "TextureD3D11.h"
#include "MemoryD3D11.h"

using namespace nri;

Result TextureD3D11::Create(const MemoryD3D11* memory)
{
    const DxgiFormat& dxgiFormat = GetDxgiFormat(m_Desc.format);

    uint32_t bindFlags = 0;
    if (m_Desc.usageMask & TextureUsageBits::SHADER_RESOURCE)
        bindFlags |= D3D11_BIND_SHADER_RESOURCE;
    if (m_Desc.usageMask & TextureUsageBits::SHADER_RESOURCE_STORAGE)
        bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    if (m_Desc.usageMask & TextureUsageBits::COLOR_ATTACHMENT)
        bindFlags |= D3D11_BIND_RENDER_TARGET;
    if (m_Desc.usageMask & TextureUsageBits::DEPTH_STENCIL_ATTACHMENT)
        bindFlags |= D3D11_BIND_DEPTH_STENCIL;

    uint32_t cpuAccessFlags = D3D11_CPU_ACCESS_READ;
    D3D11_USAGE usage = D3D11_USAGE_STAGING;
    if (memory)
    {
        usage = memory->GetType() == MemoryLocation::HOST_UPLOAD ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
        cpuAccessFlags = 0;
    }

    HRESULT hr = E_INVALIDARG;
    if (m_Desc.type == TextureType::TEXTURE_1D)
    {
        D3D11_TEXTURE1D_DESC desc = {};
        desc.Width = m_Desc.width;
        desc.MipLevels = m_Desc.mipNum;
        desc.ArraySize = m_Desc.arraySize;
        desc.Format = dxgiFormat.typeless;
        desc.Usage = usage;
        desc.BindFlags = bindFlags;
        desc.CPUAccessFlags = cpuAccessFlags;

        hr = m_Device.GetDevice()->CreateTexture1D(&desc, nullptr, (ID3D11Texture1D**)&m_Texture);
    }
    else if (m_Desc.type == TextureType::TEXTURE_3D)
    {
        D3D11_TEXTURE3D_DESC desc = {};
        desc.Width = m_Desc.width;
        desc.Height = m_Desc.height;
        desc.Depth = m_Desc.depth;
        desc.MipLevels = m_Desc.mipNum;
        desc.Format = dxgiFormat.typeless;
        desc.Usage = usage;
        desc.BindFlags = bindFlags;
        desc.CPUAccessFlags = cpuAccessFlags;

        hr = m_Device.GetDevice()->CreateTexture3D(&desc, nullptr, (ID3D11Texture3D**)&m_Texture);
    }
    else
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = m_Desc.width;
        desc.Height = m_Desc.height;
        desc.MipLevels = m_Desc.mipNum;
        desc.ArraySize = m_Desc.arraySize;
        desc.Format = dxgiFormat.typeless;
        desc.SampleDesc.Count = m_Desc.sampleNum;
        desc.Usage = usage;
        desc.BindFlags = bindFlags;
        desc.CPUAccessFlags = cpuAccessFlags;

        if (m_Desc.sampleNum == 1 && desc.Width == desc.Height && (m_Desc.arraySize % 6 == 0))
            desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        hr = m_Device.GetDevice()->CreateTexture2D(&desc, nullptr, (ID3D11Texture2D**)&m_Texture);
    }

    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateTextureXx()");

    uint64_t size = GetMipmappedSize();
    uint32_t priority = memory ? memory->GetResidencyPriority(size) : 0;
    if (priority != 0)
        m_Texture->SetEvictionPriority(priority);

    return Result::SUCCESS;
}

Result TextureD3D11::Create(const TextureD3D11Desc& textureDesc)
{
    if (!GetTextureDesc(textureDesc, m_Desc))
        return Result::INVALID_ARGUMENT;

    m_Texture = (ID3D11Resource*)textureDesc.d3d11Resource;

    return Result::SUCCESS;
}

uint32_t TextureD3D11::GetMipmappedSize(uint32_t w, uint32_t h, uint32_t d, uint32_t mipOffset, uint32_t mipNum) const
{
    if (!mipNum)
        mipNum = m_Desc.mipNum;

    bool isCompressed = m_Desc.format >= Format::BC1_RGBA_UNORM && m_Desc.format <= Format::BC7_RGBA_SRGB;
    bool isCustom = w || h || d;

    if (!w)
        w = GetSize(0, mipOffset);

    if (!h)
        h = GetSize(1, mipOffset);

    if (!d)
        d = GetSize(2, mipOffset);

    uint32_t size = 0;

    while (mipNum)
    {
        if (isCompressed)
            size += ((w + 3) >> 2) * ((h + 3) >> 2) * d;
        else
            size += w * h * d;

        if (w == 1 && h == 1 && d == 1)
            break;

        if (d > 1)
            d >>= 1;

        if (w > 1)
            w >>= 1;

        if (h > 1)
            h >>= 1;

        mipNum--;
    }

    const FormatInfo& formatInfo = GetFormatProps(m_Desc.format);
    size *= formatInfo.stride;

    if (!isCustom)
    {
        size *= m_Desc.sampleNum;
        size *= m_Desc.arraySize;
    }

    return size;
}

uint16_t TextureD3D11::GetSize(uint32_t dimension, uint32_t mipOffset) const
{
    assert(dimension < 3);

    uint16_t dim = m_Desc.depth;
    if (dimension == 0)
        dim = m_Desc.width;
    else if (dimension == 1)
        dim = m_Desc.height;

    dim = (uint16_t)std::max(dim >> mipOffset, 1);
    dim = Align(dim, dimension < 2 ? (uint16_t)GetFormatProps(m_Desc.format).blockWidth : 1);

    return dim;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void TextureD3D11::GetMemoryInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const
{
    bool isMultisampled = m_Desc.sampleNum > 1;
    uint32_t size = GetMipmappedSize();

    uint32_t alignment = 65536;
    if (isMultisampled)
        alignment = 4194304;
    else if (size <= 65536)
        alignment = 65536;

    size = Align(size, alignment);

    memoryDesc.type = (MemoryType)memoryLocation;
    memoryDesc.size = size;
    memoryDesc.alignment = alignment;
    memoryDesc.mustBeDedicated = false;
}

#include "TextureD3D11.hpp"
