// © 2021 NVIDIA Corporation

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
    desc.Layout = textureDesc.tiled ? D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE : D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = GetTextureFlags(textureDesc.usageMask);

    return desc;
}

SubresourceTilingDesc nri::TextureD3D12::GetSubresourceTiling(Dim_t arrayOffset, Mip_t mipOffset) const
{ 
    const uint32_t subresourceIndex = GetSubresourceIndex(arrayOffset, mipOffset);
    RETURN_ON_FAILURE(&m_Device, subresourceIndex < m_SubresourceTilings.size(), {}, "Trying to access unexisting or packed subresource");
    return m_SubresourceTilings.at(subresourceIndex);
}

inline TextureTilingRequirementsDesc nri::TextureD3D12::GetTilingRequirements() const 
{
    return m_TilingRequirements;
}

void nri::TextureD3D12::ObtainTilingInfo() 
{
    UINT numTiles = 0;
    D3D12_PACKED_MIP_INFO packedMipInfo = {};
    D3D12_TILE_SHAPE tileShape = {};
    UINT subresourceCount = m_Desc.arraySize * m_Desc.mipNum;
    D3D12_SUBRESOURCE_TILING* tilings = STACK_ALLOC(D3D12_SUBRESOURCE_TILING, subresourceCount);
    ((ID3D12Device*)m_Device)->GetResourceTiling(m_Texture, &numTiles, &packedMipInfo, &tileShape, &subresourceCount, 0, tilings);

    m_TilingRequirements.mipPackedNum = packedMipInfo.NumPackedMips;
    m_TilingRequirements.mipStandardNum = packedMipInfo.NumStandardMips;
    m_TilingRequirements.packedTileNum = packedMipInfo.NumTilesForPackedMips;
    m_TilingRequirements.startPackedTileIndex = packedMipInfo.StartTileIndexInOverallResource;
    m_TilingRequirements.tileWidth = tileShape.WidthInTexels;
    m_TilingRequirements.tileHeight = tileShape.HeightInTexels;
    m_TilingRequirements.tileDepth = tileShape.DepthInTexels;

    m_SubresourceTilings.reserve(packedMipInfo.NumStandardMips);
    for (size_t i = 0; i < packedMipInfo.NumStandardMips; i++) {
        const D3D12_SUBRESOURCE_TILING& tiling = tilings[i];
        m_SubresourceTilings.emplace_back(
            SubresourceTilingDesc{tiling.WidthInTiles, tiling.HeightInTiles, tiling.DepthInTiles, tiling.StartTileIndexInOverallResource}
        );
    }
}

Result TextureD3D12::Create(const TextureDesc& textureDesc)
{
    if (textureDesc.tiled)
    {
        D3D12_RESOURCE_DESC desc = GetResourceDesc(textureDesc);
        D3D12_CLEAR_VALUE clearValue = {GetDxgiFormat(textureDesc.format).typed};
        bool isRenderableSurface = desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        RETURN_ON_FAILURE(&m_Device, m_Device.GetDesc().isTilingSupported, Result::UNSUPPORTED, "Tiled textures are not supported on this device");
        HRESULT hr = ((ID3D12Device*)m_Device)->CreateReservedResource(
            &desc, 
            D3D12_RESOURCE_STATE_COMMON, 
            isRenderableSurface ? &clearValue : nullptr,
            IID_PPV_ARGS(&m_Texture)
        );

        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateReservedResource()");
    }

    m_Desc = textureDesc;
    if (m_Desc.tiled)
        ObtainTilingInfo();

    return Result::SUCCESS;
}

Result TextureD3D12::Create(const TextureD3D12Desc& textureDesc)
{
    if (!GetTextureDesc(textureDesc, m_Desc))
        return Result::INVALID_ARGUMENT;

    m_Texture = textureDesc.d3d12Resource;
    if (m_Desc.tiled)
        ObtainTilingInfo();

    return Result::SUCCESS;
}

Result TextureD3D12::BindMemory(const MemoryD3D12* memory, uint64_t offset)
{
    RETURN_ON_FAILURE(&m_Device, !m_Desc.tiled, Result::INVALID_ARGUMENT, "Tile textures can't be binded directly. Use \"TilingInteface\" for such purposes.");

    D3D12_RESOURCE_DESC desc = GetResourceDesc(m_Desc);
    D3D12_CLEAR_VALUE clearValue = { GetDxgiFormat(m_Desc.format).typed };
    bool isRenderableSurface = desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    if (memory->RequiresDedicatedAllocation()) 
    {
        const D3D12_HEAP_DESC& heapDesc = memory->GetHeapDesc();
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
