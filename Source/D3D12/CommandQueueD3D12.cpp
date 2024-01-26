// © 2021 NVIDIA Corporation

#include "SharedD3D12.h"
#include "TextureD3D12.h"
#include "MemoryD3D12.h"
#include "CommandQueueD3D12.h"
#include "CommandBufferD3D12.h"

using namespace nri;

Result CommandQueueD3D12::Create(CommandQueueType queueType)
{
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.NodeMask = NRI_TEMP_NODE_MASK;
    commandQueueDesc.Type = GetCommandListType(queueType);

    HRESULT hr = ((ID3D12Device*)m_Device)->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_CommandQueue));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommandQueue()");

    m_CommandListType = commandQueueDesc.Type;

    return Result::SUCCESS;
}

Result CommandQueueD3D12::Create(ID3D12CommandQueue* commandQueue)
{
    const D3D12_COMMAND_QUEUE_DESC& commandQueueDesc = commandQueue->GetDesc();

    m_CommandQueue = commandQueue;
    m_CommandListType = commandQueueDesc.Type;

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void CommandQueueD3D12::Submit(const QueueSubmitDesc& queueSubmitDesc)
{
    if (queueSubmitDesc.commandBufferNum)
    {
        ID3D12CommandList** commandLists = STACK_ALLOC(ID3D12CommandList*, queueSubmitDesc.commandBufferNum);
        for (uint32_t j = 0; j < queueSubmitDesc.commandBufferNum; j++)
            commandLists[j] = *(CommandBufferD3D12*)queueSubmitDesc.commandBuffers[j];

        m_CommandQueue->ExecuteCommandLists(queueSubmitDesc.commandBufferNum, commandLists);
    }
}

inline Result CommandQueueD3D12::ChangeResourceStates(const TransitionBarrierDesc& transitionBarriers)
{
    HelperResourceStateChange resourceStateChange(m_Device.GetCoreInterface(), (Device&)m_Device, (CommandQueue&)*this);

    return resourceStateChange.ChangeStates(transitionBarriers);
}

inline Result CommandQueueD3D12::UploadData(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum,
    const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum)
{
    HelperDataUpload helperDataUpload(m_Device.GetCoreInterface(), (Device&)m_Device, m_Device.GetStdAllocator(), (CommandQueue&)*this);

    return helperDataUpload.UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

inline Result CommandQueueD3D12::WaitForIdle()
{
    HelperWaitIdle helperWaitIdle(m_Device.GetCoreInterface(), (Device&)m_Device, (CommandQueue&)*this);

    return helperWaitIdle.WaitIdle();
}

inline Result CommandQueueD3D12::BindTextureTiles(Texture& texture, const TileBindDesc* binds, uint32_t bindNum) 
{
    TextureD3D12& nativeTexture = (TextureD3D12&)texture;
    TextureTilingRequirementsDesc tilingRequirements = nativeTexture.GetTilingRequirements();

    // TODO: batch multiple calls of UpdateTileMappings into one
    for (size_t i = 0; i < bindNum; i++) 
    {
        const TileBindDesc& bind = binds[i];
        D3D12_TILE_REGION_SIZE tileRegion = {};
        uint32_t subresource = nativeTexture.GetSubresourceIndex(bind.region.arrayOffset, bind.region.mipOffset);
        D3D12_TILED_RESOURCE_COORDINATE resourceCoordinate = {};
        if (nativeTexture.IsPackedSubresource(subresource))
        {
            // Don't use any offsets here, just update an entire packed mip
            resourceCoordinate.Subresource = tilingRequirements.startPackedTileIndex;
            tileRegion.NumTiles = tilingRequirements.packedTileNum;
            tileRegion.UseBox = FALSE;
        }
        else
        {
            resourceCoordinate.Subresource = subresource;
            resourceCoordinate.X = bind.region.x / tilingRequirements.tileWidth;
            resourceCoordinate.Y = bind.region.y / tilingRequirements.tileHeight;
            resourceCoordinate.Z = bind.region.z / tilingRequirements.tileDepth;
            tileRegion.Width = bind.region.width / tilingRequirements.tileWidth;
            tileRegion.Height = bind.region.height / tilingRequirements.tileHeight;
            tileRegion.Depth = bind.region.depth / tilingRequirements.tileDepth;
            tileRegion.NumTiles = tileRegion.Width * tileRegion.Height * tileRegion.Depth;
            tileRegion.UseBox = TRUE;
        }

        D3D12_TILE_RANGE_FLAGS rangeFlag = bind.tilePool == nullptr ? D3D12_TILE_RANGE_FLAG_NONE : D3D12_TILE_RANGE_FLAG_NULL;
        UINT heapTiledOffset = bind.offsetInTiles;

        MemoryD3D12* memory = (MemoryD3D12*)bind.tilePool;
        m_CommandQueue->UpdateTileMappings(
            nativeTexture, 
            1,
            &resourceCoordinate, 
            &tileRegion, 
            *memory, 
            1, 
            &rangeFlag,
            &heapTiledOffset,
            &tileRegion.NumTiles,
            D3D12_TILE_MAPPING_FLAG_NONE
        );
    }
}

#include "CommandQueueD3D12.hpp"
