#pragma once

constexpr size_t BASE_UPLOAD_BUFFER_SIZE = 1 * 1024 * 1024;

struct HelperDataUpload {
    inline HelperDataUpload(const nri::CoreInterface& NRI, nri::Device& device, nri::CommandQueue& commandQueue) :
        NRI(NRI), m_Device(device), m_CommandQueue(commandQueue), m_UploadBufferSize(BASE_UPLOAD_BUFFER_SIZE) {
    }

    nri::Result UploadData(const nri::TextureUploadDesc* textureDataDescs, uint32_t textureDataDescNum, const nri::BufferUploadDesc* bufferDataDescs, uint32_t bufferDataDescNum);

private:
    nri::Result Create();
    nri::Result UploadTextures(const nri::TextureUploadDesc* textureDataDescs, uint32_t textureDataDescNum);
    nri::Result UploadBuffers(const nri::BufferUploadDesc* bufferDataDescs, uint32_t bufferDataDescNum);
    nri::Result EndCommandBuffersAndSubmit();
    bool CopyTextureContent(const nri::TextureUploadDesc& textureDataDesc, nri::Dim_t& arrayOffset, nri::Mip_t& mipOffset, bool& isCapacityInsufficient);
    void CopyTextureSubresourceContent(const nri::TextureSubresourceUploadDesc& subresource, uint64_t alignedRowPitch, uint64_t alignedSlicePitch);
    bool CopyBufferContent(const nri::BufferUploadDesc& bufferDataDesc, uint64_t& bufferContentOffset);

    const nri::CoreInterface& NRI;
    nri::Device& m_Device;
    nri::CommandQueue& m_CommandQueue;
    nri::CommandBuffer* m_CommandBuffer = nullptr;
    nri::Fence* m_Fence = nullptr;
    nri::CommandAllocator* m_CommandAllocators = nullptr;
    nri::Buffer* m_UploadBuffer = nullptr;
    nri::Memory* m_UploadBufferMemory = nullptr;
    uint8_t* m_MappedMemory = nullptr;
    uint64_t m_UploadBufferSize = 0;
    uint64_t m_UploadBufferOffset = 0;
    uint64_t m_FenceValue = 1;
};