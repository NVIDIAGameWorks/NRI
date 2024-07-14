// © 2021 NVIDIA Corporation

#include "SharedD3D12.h"

#include "MemoryD3D12.h"
#include "TextureD3D12.h"

using namespace nri;

static inline void GetResourceDesc(D3D12_RESOURCE_DESC* desc, const TextureDesc& textureDesc) {
    uint16_t blockWidth = (uint16_t)GetFormatProps(textureDesc.format).blockWidth;

    desc->Dimension = GetResourceDimension(textureDesc.type);
    desc->Alignment = textureDesc.sampleNum > 1 ? 0 : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc->Width = Align(textureDesc.width, blockWidth);
    desc->Height = Align(textureDesc.height, blockWidth);
    desc->DepthOrArraySize = textureDesc.type == TextureType::TEXTURE_3D ? textureDesc.depth : textureDesc.arraySize;
    desc->MipLevels = textureDesc.mipNum;
    desc->Format = GetDxgiFormat(textureDesc.format).typeless;
    desc->SampleDesc.Count = textureDesc.sampleNum;
    desc->Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc->Flags = GetTextureFlags(textureDesc.usageMask);
}

Result TextureD3D12::Create(const TextureDesc& textureDesc) {
    m_Desc = textureDesc;

    return Result::SUCCESS;
}

Result TextureD3D12::Create(const TextureD3D12Desc& textureDesc) {
    if (textureDesc.desc)
        m_Desc = *textureDesc.desc;
    else if (!GetTextureDesc(textureDesc, m_Desc))
        return Result::INVALID_ARGUMENT;

    m_Texture = (ID3D12ResourceBest*)textureDesc.d3d12Resource;

    return Result::SUCCESS;
}

Result TextureD3D12::BindMemory(const MemoryD3D12* memory, uint64_t offset) {
    // Texture was already created externally
    if (m_Texture)
        return Result::SUCCESS;

    const D3D12_HEAP_DESC& heapDesc = memory->GetHeapDesc();
    D3D12_CLEAR_VALUE clearValue = {GetDxgiFormat(m_Desc.format).typed};

#ifdef NRI_USE_AGILITY_SDK
    if (m_Device.GetVersion() >= 10) {
        D3D12_RESOURCE_DESC1 desc1 = {};
        GetResourceDesc((D3D12_RESOURCE_DESC*)&desc1, m_Desc);

        bool isRenderableSurface = desc1.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
        D3D12_BARRIER_LAYOUT initialLayout = D3D12_BARRIER_LAYOUT_COMMON;
        uint32_t castableFormatNum = 0;
        DXGI_FORMAT* castableFormats = nullptr; // TODO: add castable formats, see options12.RelaxedFormatCastingSupported

        if (memory->RequiresDedicatedAllocation()) {
            HRESULT hr = m_Device->CreateCommittedResource3(&heapDesc.Properties, heapDesc.Flags, &desc1, initialLayout, isRenderableSurface ? &clearValue : nullptr, nullptr,
                castableFormatNum, castableFormats, IID_PPV_ARGS(&m_Texture));
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device10::CreateCommittedResource3()");
        } else {
            HRESULT hr = m_Device->CreatePlacedResource2(
                *memory, offset, &desc1, initialLayout, isRenderableSurface ? &clearValue : nullptr, castableFormatNum, castableFormats, IID_PPV_ARGS(&m_Texture));
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device10::CreatePlacedResource2()");
        }
    } else
#endif
    {
        D3D12_RESOURCE_DESC desc = {};
        GetResourceDesc(&desc, m_Desc);

        bool isRenderableSurface = desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        if (memory->RequiresDedicatedAllocation()) {
            HRESULT hr = m_Device->CreateCommittedResource(
                &heapDesc.Properties, heapDesc.Flags, &desc, D3D12_RESOURCE_STATE_COMMON, isRenderableSurface ? &clearValue : nullptr, IID_PPV_ARGS(&m_Texture));
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommittedResource()");
        } else {
            HRESULT hr = m_Device->CreatePlacedResource(*memory, offset, &desc, D3D12_RESOURCE_STATE_COMMON, isRenderableSurface ? &clearValue : nullptr, IID_PPV_ARGS(&m_Texture));
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreatePlacedResource()");
        }
    }

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void TextureD3D12::GetMemoryInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    D3D12_RESOURCE_DESC desc = {};
    GetResourceDesc(&desc, m_Desc);

    m_Device.GetMemoryInfo(memoryLocation, desc, memoryDesc);
}

#include "TextureD3D12.hpp"
