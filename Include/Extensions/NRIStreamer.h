// © 2024 NVIDIA Corporation

#pragma once

NRI_NAMESPACE_BEGIN

NRI_FORWARD_STRUCT(Streamer);

NRI_STRUCT(StreamerDesc)
{    
    // Statically allocated ring-buffer for dynamic constants
    NRI_OPTIONAL NRI_NAME(MemoryLocation) constantBufferMemoryLocation; // UPLOAD or DEVICE_UPLOAD
    NRI_OPTIONAL uint64_t constantBufferSize;

    // Dynamically (re)allocated ring-buffer for copying and rendering (mandatory)
    NRI_NAME(MemoryLocation) dynamicBufferMemoryLocation; // UPLOAD or DEVICE_UPLOAD
    NRI_NAME(BufferUsageBits) dynamicBufferUsageBits;
    uint32_t frameInFlightNum;
};

NRI_STRUCT(BufferUpdateRequestDesc)
{
    // Data to upload
    const void* data; // pointer must be valid until "CopyStreamerUpdateRequests" call
    uint64_t dataSize;

    // Destination (ignored for constants)
    NRI_OPTIONAL NRI_NAME(Buffer)* dstBuffer;
    NRI_OPTIONAL uint64_t dstBufferOffset;
};

NRI_STRUCT(TextureUpdateRequestDesc)
{
    // Data to upload
    const void* data; // pointer must be valid until "CopyStreamerUpdateRequests" call
    uint32_t dataRowPitch;
    uint32_t dataSlicePitch;

    // Destination
    NRI_NAME(Texture)* dstTexture;
    NRI_NAME(TextureRegionDesc) dstRegionDesc;
};

NRI_STRUCT(StreamerInterface)
{
    NRI_NAME(Result) (NRI_CALL *CreateStreamer)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(StreamerDesc) streamerDesc, NRI_NAME_REF(Streamer*) streamer);
    void (NRI_CALL *DestroyStreamer)(NRI_NAME_REF(Streamer) streamer);

    // Get internal buffers
    NRI_NAME(Buffer*) (NRI_CALL *GetStreamerConstantBuffer)(NRI_NAME_REF(Streamer) streamer); // Never changes
    NRI_NAME(Buffer*) (NRI_CALL *GetStreamerDynamicBuffer)(NRI_NAME_REF(Streamer) streamer); // Valid only after "CopyStreamerUpdateRequests"

    // Add an update request. Return the offset in the ring buffer and don't invoke any work
    uint64_t (NRI_CALL *AddStreamerBufferUpdateRequest)(NRI_NAME_REF(Streamer) streamer, const NRI_NAME_REF(BufferUpdateRequestDesc) bufferUpdateRequestDesc);
    uint64_t (NRI_CALL *AddStreamerTextureUpdateRequest)(NRI_NAME_REF(Streamer) streamer, const NRI_NAME_REF(TextureUpdateRequestDesc) textureUpdateRequestDesc);

    // (HOST) Copy data and get the offset in the dedicated ring buffer (for dynamic constant buffers)
    uint32_t (NRI_CALL *UpdateStreamerConstantBuffer)(NRI_NAME_REF(Streamer) streamer, const void* data, uint32_t dataSize);

    // (HOST) Copy gathered requests to the internal buffer, potentially a new one if the capacity exceeded. Must be called once per frame
    NRI_NAME(Result) (NRI_CALL *CopyStreamerUpdateRequests)(NRI_NAME_REF(Streamer) streamer);

    // (DEVICE) Copy data to destinations (if any), barriers are externally controlled. Must be called after "CopyStreamerUpdateRequests"
    // WARNING: D3D12 can silently promote a resource state to COPY_DESTINATION!
    void (NRI_CALL *CmdUploadStreamerUpdateRequests)(NRI_NAME_REF(CommandBuffer) commandBuffer, NRI_NAME_REF(Streamer) streamer);
};

NRI_NAMESPACE_END