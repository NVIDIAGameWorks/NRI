#pragma once

NRI_NAMESPACE_BEGIN

NRI_STRUCT(TilePool);

NRI_STRUCT(SubresourceTilingDesc)
{
    uint32_t widthInTiles;
    uint16_t heightInTiles;
    uint16_t depthInTiles;
    uint32_t resourceOffsetInTiles;
};

NRI_STRUCT(TileBindDesc) 
{
    NRI_NAME(TextureRegionDesc) region; // all sizes in pixels, not tiles!
    NRI_NAME(TilePool)* tilePool;
    uint64_t offsetInTiles;
};

NRI_STRUCT(TextureTilingRequirementsDesc)
{
    NRI_NAME(Dim_t) tileWidth;
    NRI_NAME(Dim_t) tileHeight;
    NRI_NAME(Dim_t) tileDepth;
    NRI_NAME(Mip_t) mipStandardNum;
    NRI_NAME(Mip_t) mipPackedNum;
    uint32_t packedTileNum;
    uint32_t startPackedTileIndex;
};

/*
    Result(NRI_CALL* GetTextureTiling)(Device& device, NRI_NAME_REF(Texture) texture, uint32_t* tileNum, const NRI_NAME_REF(PackedMipInfoDesc) packedMipInfo,
   NRI_NAME_REF(TileShapeDesc), uint32_t subresourceNum, const NRI_NAME_REF(SubresourceTilingDesc) subresourceTilings);

*/

NRI_STRUCT(TilingInterface)
{
    Result(NRI_CALL* CreateTilePool)(Device& device, uint32_t nodeMask, NRI_NAME(MemoryType) memoryType, uint64_t poolSize, NRI_NAME_REF(TilePool*) tilePool);
    void(NRI_CALL* DestroyTilePool)(Device& device, NRI_NAME_REF(TilePool) tilePool);

    void(NRI_CALL* GetSubresourceTiling)(Device& device, uint32_t subresource, NRI_NAME_REF(SubresourceTilingDesc) subresourceTiling);
    void(NRI_CALL * GetTextureTilingRequirements)(Device & device, NRI_NAME_REF(TextureTilingRequirementsDesc) memoryRequirements);

    Result(NRI_CALL* BindTextureTiles)(Device& device, NRI_NAME_REF(CommandQueue) commandQueue, NRI_NAME_REF(Texture) texture, const NRI_NAME(TileBindDesc)* binds, uint32_t bindNum);

};

NRI_NAMESPACE_END
