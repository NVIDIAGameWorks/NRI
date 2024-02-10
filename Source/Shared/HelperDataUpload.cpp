#include "SharedExternal.h"

using namespace nri;

constexpr size_t BASE_UPLOAD_BUFFER_SIZE = 65536;

HelperDataUpload::HelperDataUpload(const CoreInterface& NRI, Device& device, const StdAllocator<uint8_t>& stdAllocator, CommandQueue& commandQueue) :
    NRI(NRI),
    m_Device(device),
    m_CommandQueue(commandQueue),
    m_CommandBuffers(stdAllocator),
    m_UploadBufferSize(BASE_UPLOAD_BUFFER_SIZE)
{
}

Result HelperDataUpload::UploadData(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum,
    const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum)
{
    const DeviceDesc& deviceDesc = NRI.GetDeviceDesc(m_Device);

    for (uint32_t i = 0; i < textureUploadDescNum; i++)
    {
        if (!textureUploadDescs[i].subresources)
            continue;

        const TextureSubresourceUploadDesc& subresource = textureUploadDescs[i].subresources[0];

        const uint32_t sliceRowNum = std::max(subresource.slicePitch / subresource.rowPitch, 1u);
        const uint64_t alignedRowPitch = Align(subresource.rowPitch, deviceDesc.uploadBufferTextureRowAlignment);
        const uint64_t alignedSlicePitch = Align(sliceRowNum * alignedRowPitch, deviceDesc.uploadBufferTextureSliceAlignment);
        const uint64_t mipLevelContentSize = alignedSlicePitch * std::max(subresource.sliceNum, 1u);
        m_UploadBufferSize = std::max(m_UploadBufferSize, mipLevelContentSize);
    }

    m_UploadBufferSize = Align(m_UploadBufferSize, COPY_ALIGMENT);

    Result result = Create();

    if (result != Result::SUCCESS)
        return result;

    result = UploadTextures(textureUploadDescs, textureUploadDescNum);

    if (result == Result::SUCCESS)
        result = UploadBuffers(bufferUploadDescs, bufferUploadDescNum);

    Destroy();

    return result;
}

Result HelperDataUpload::Create()
{
    BufferDesc bufferDesc = {};
    bufferDesc.size = m_UploadBufferSize;
    Result result = NRI.CreateBuffer(m_Device, bufferDesc, m_UploadBuffer);
    if (result != Result::SUCCESS)
        return result;

    MemoryDesc memoryDesc = {};
    NRI.GetBufferMemoryInfo(*m_UploadBuffer, MemoryLocation::HOST_UPLOAD, memoryDesc);

    result = NRI.AllocateMemory(m_Device, ALL_NODES, memoryDesc.type, memoryDesc.size, m_UploadBufferMemory);
    if (result != Result::SUCCESS)
        return result;

    const BufferMemoryBindingDesc bufferMemoryBindingDesc = { m_UploadBufferMemory, m_UploadBuffer, 0 };
    result = NRI.BindBufferMemory(m_Device, &bufferMemoryBindingDesc, 1);
    if (result != Result::SUCCESS)
        return result;

    result = NRI.CreateFence(m_Device, 0, m_Fence);
    if (result != Result::SUCCESS)
        return result;

    const DeviceDesc& deviceDesc = NRI.GetDeviceDesc(m_Device);
    m_CommandBuffers.resize(deviceDesc.nodeNum);

    result = NRI.CreateCommandAllocator(m_CommandQueue, m_CommandAllocators);
    if (result != Result::SUCCESS)
        return result;

    for (uint32_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        result = NRI.CreateCommandBuffer(*m_CommandAllocators, m_CommandBuffers[i]);
        if (result != Result::SUCCESS)
            return result;
    }

    return result;
}

void HelperDataUpload::Destroy()
{
    for (uint32_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        NRI.DestroyCommandBuffer(*m_CommandBuffers[i]);
        NRI.DestroyCommandAllocator(*m_CommandAllocators);
    }

    NRI.DestroyFence(*m_Fence);
    NRI.DestroyBuffer(*m_UploadBuffer);
    NRI.FreeMemory(*m_UploadBufferMemory);
}

Result HelperDataUpload::UploadTextures(const TextureUploadDesc* textureUploadDescs, uint32_t textureDataDescNum)
{
    if (!textureDataDescNum)
        return Result::SUCCESS;

    bool firstTime = true;
    uint32_t i = 0;
    Dim_t arrayOffset = 0;
    Mip_t mipOffset = 0;
    Result result;

    while (i < textureDataDescNum)
    {
        if (!firstTime)
        {
            result = EndCommandBuffersAndSubmit();
            if (result != Result::SUCCESS)
                return result;
        }
        
        result = BeginCommandBuffers();
        if (result != Result::SUCCESS)
            return result;

        if (firstTime)
        {
            DoTransition<true>(textureUploadDescs, textureDataDescNum);
            firstTime = false;
        }

        m_UploadBufferOffset = 0;
        bool isCapacityInsufficient = false;

        for (; i < textureDataDescNum && CopyTextureContent(textureUploadDescs[i], arrayOffset, mipOffset, isCapacityInsufficient); i++)
            ;

        if (isCapacityInsufficient)
            return Result::OUT_OF_MEMORY;
    }

    DoTransition<false>(textureUploadDescs, textureDataDescNum);

    return EndCommandBuffersAndSubmit();
}

Result HelperDataUpload::UploadBuffers(const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum)
{
    if (!bufferUploadDescNum)
        return Result::SUCCESS;

    bool firstTime = true;
    uint32_t i = 0;
    uint64_t bufferContentOffset = 0;
    Result result;

    while (i < bufferUploadDescNum)
    {
        if (!firstTime)
        {
            result = EndCommandBuffersAndSubmit();
            if (result != Result::SUCCESS)
                return result;
        }
        
        result = BeginCommandBuffers();
        if (result != Result::SUCCESS)
            return result;

        if (firstTime)
        {
            DoTransition<true>(bufferUploadDescs, bufferUploadDescNum);
            firstTime = false;
        }

        m_UploadBufferOffset = 0;
        m_MappedMemory = (uint8_t*)NRI.MapBuffer(*m_UploadBuffer, 0, m_UploadBufferSize);

        for (; i < bufferUploadDescNum && CopyBufferContent(bufferUploadDescs[i], bufferContentOffset); i++)
            ;

        NRI.UnmapBuffer(*m_UploadBuffer);
    }

    DoTransition<false>(bufferUploadDescs, bufferUploadDescNum);

    return EndCommandBuffersAndSubmit();
}

Result HelperDataUpload::BeginCommandBuffers()
{
    Result result = Result::SUCCESS;

    for (uint32_t i = 0; i < m_CommandBuffers.size() && result == Result::SUCCESS; i++)
        result = NRI.BeginCommandBuffer(*m_CommandBuffers[i], nullptr, i);

    return result;
}

Result HelperDataUpload::EndCommandBuffersAndSubmit()
{
    for (uint32_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        const Result result = NRI.EndCommandBuffer(*m_CommandBuffers[i]);
        if (result != Result::SUCCESS)
            return result;
    }

    QueueSubmitDesc queueSubmitDesc = {};
    queueSubmitDesc.commandBufferNum = 1;

    for (uint32_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        queueSubmitDesc.commandBuffers = &m_CommandBuffers[i];
        queueSubmitDesc.nodeIndex = i;

        NRI.QueueSubmit(m_CommandQueue, queueSubmitDesc);

        NRI.QueueSignal(m_CommandQueue, *m_Fence, m_FenceValue);
        NRI.Wait(*m_Fence, m_FenceValue);

        m_FenceValue++;
    }

    NRI.ResetCommandAllocator(*m_CommandAllocators);

    return Result::SUCCESS;
}

bool HelperDataUpload::CopyTextureContent(const TextureUploadDesc& textureUploadDesc, Dim_t& arrayOffset, Mip_t& mipOffset, bool& isCapacityInsufficient)
{
    if (textureUploadDesc.subresources == nullptr)
        return true;

    const DeviceDesc& deviceDesc = NRI.GetDeviceDesc(m_Device);
    const TextureDesc& textureDesc = NRI.GetTextureDesc(*textureUploadDesc.texture);

    for (; arrayOffset < textureDesc.arraySize; arrayOffset++)
    {
        for (; mipOffset < textureDesc.mipNum; mipOffset++)
        {
            const auto& subresource = textureUploadDesc.subresources[arrayOffset * textureDesc.mipNum + mipOffset];

            const uint32_t sliceRowNum = subresource.slicePitch / subresource.rowPitch;
            const uint32_t alignedRowPitch = Align(subresource.rowPitch, deviceDesc.uploadBufferTextureRowAlignment);
            const uint32_t alignedSlicePitch = Align(sliceRowNum * alignedRowPitch, deviceDesc.uploadBufferTextureSliceAlignment);
            const uint64_t mipLevelContentSize = uint64_t(alignedSlicePitch) * subresource.sliceNum;
            const uint64_t freeSpace = m_UploadBufferSize - m_UploadBufferOffset;

            if (mipLevelContentSize > freeSpace)
            {
                isCapacityInsufficient = mipLevelContentSize > m_UploadBufferSize;
                return false;
            }

            CopyTextureSubresourceContent(subresource, alignedRowPitch, alignedSlicePitch);

            TextureDataLayoutDesc srcDataLayout = {};
            srcDataLayout.offset = m_UploadBufferOffset;
            srcDataLayout.rowPitch = alignedRowPitch;
            srcDataLayout.slicePitch = alignedSlicePitch;

            TextureRegionDesc dstRegion = {};
            dstRegion.arrayOffset = arrayOffset;
            dstRegion.mipOffset = mipOffset;

            for (uint32_t k = 0; k < m_CommandBuffers.size(); k++)
                NRI.CmdUploadBufferToTexture(*m_CommandBuffers[k], *textureUploadDesc.texture, dstRegion, *m_UploadBuffer, srcDataLayout);

            m_UploadBufferOffset = Align(m_UploadBufferOffset + mipLevelContentSize, COPY_ALIGMENT);
        }
        mipOffset = 0;
    }
    arrayOffset = 0;

    m_UploadBufferOffset = Align(m_UploadBufferOffset, COPY_ALIGMENT);

    return true;
}

void HelperDataUpload::CopyTextureSubresourceContent(const TextureSubresourceUploadDesc& subresource, uint64_t alignedRowPitch, uint64_t alignedSlicePitch)
{
    const uint32_t sliceRowNum = subresource.slicePitch / subresource.rowPitch;

    // TODO: D3D11 does not allow to call CmdUploadBufferToTexture() while the upload buffer is mapped.
    m_MappedMemory = (uint8_t*)NRI.MapBuffer(*m_UploadBuffer, m_UploadBufferOffset, subresource.sliceNum * alignedSlicePitch);

    uint8_t* slices = m_MappedMemory;
    for (uint32_t k = 0; k < subresource.sliceNum; k++)
    {
        for (uint32_t l = 0; l < sliceRowNum; l++)
        {
            uint8_t* dstRow = slices + k * alignedSlicePitch + l * alignedRowPitch;
            uint8_t* srcRow = (uint8_t*)subresource.slices + k * subresource.slicePitch + l * subresource.rowPitch;
            memcpy(dstRow, srcRow, subresource.rowPitch);
        }
    }

    NRI.UnmapBuffer(*m_UploadBuffer);
}

bool HelperDataUpload::CopyBufferContent(const BufferUploadDesc& bufferUploadDesc, uint64_t& bufferContentOffset)
{
    if (bufferUploadDesc.dataSize == 0)
        return true;

    const uint64_t freeSpace = m_UploadBufferSize - m_UploadBufferOffset;
    const uint64_t copySize = std::min(bufferUploadDesc.dataSize - bufferContentOffset, freeSpace);

    if (freeSpace == 0)
        return false;

    memcpy(m_MappedMemory + m_UploadBufferOffset, (uint8_t*)bufferUploadDesc.data + bufferContentOffset, (size_t)copySize);

    for (uint32_t j = 0; j < m_CommandBuffers.size(); j++)
    {
        NRI.CmdCopyBuffer(*m_CommandBuffers[j], *bufferUploadDesc.buffer, j, bufferUploadDesc.bufferOffset + bufferContentOffset,
            *m_UploadBuffer, 0, m_UploadBufferOffset, copySize);
    }

    bufferContentOffset += copySize;
    m_UploadBufferOffset += copySize;

    if (bufferContentOffset != bufferUploadDesc.dataSize)
        return false;

    bufferContentOffset = 0;
    m_UploadBufferOffset = Align(m_UploadBufferOffset, COPY_ALIGMENT);

    return true;
}

template<bool isInitialTransition>
void HelperDataUpload::DoTransition(const TextureUploadDesc* textureUploadDescs, uint32_t textureDataDescNum)
{
    constexpr uint32_t TEXTURES_PER_PASS = 256;
    TextureBarrierDesc textureBarriers[TEXTURES_PER_PASS];

    AccessLayoutStage state = {AccessBits::COPY_DESTINATION, Layout::COPY_DESTINATION, StageBits::COPY};
    for (uint32_t i = 0; i < textureDataDescNum;)
    {
        const uint32_t passBegin = i;
        const uint32_t passEnd = std::min(i + TEXTURES_PER_PASS, textureDataDescNum);

        for ( ; i < passEnd; i++)
        {
            const TextureUploadDesc& textureUploadDesc = textureUploadDescs[i];
            const TextureDesc& textureDesc = NRI.GetTextureDesc(*textureUploadDesc.texture);

            TextureBarrierDesc& barrier = textureBarriers[i - passBegin];
            barrier = {};
            barrier.texture = textureUploadDesc.texture;
            barrier.mipNum = textureDesc.mipNum;
            barrier.arraySize = textureDesc.arraySize;
            barrier.before = isInitialTransition ? textureUploadDesc.before : state;
            barrier.after = isInitialTransition ? state : textureUploadDesc.after;
        }

        BarrierGroupDesc barrierGroup = {};
        barrierGroup.textures = textureBarriers;
        barrierGroup.textureNum = uint16_t(passEnd - passBegin);

        for (uint32_t j = 0; j < m_CommandBuffers.size(); j++)
            NRI.CmdBarrier(*m_CommandBuffers[j], barrierGroup);
    }
}

template<bool isInitialTransition>
void HelperDataUpload::DoTransition(const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum)
{
    constexpr uint32_t BUFFERS_PER_PASS = 256;
    BufferBarrierDesc bufferBarriers[BUFFERS_PER_PASS];

    AccessStage state = {AccessBits::COPY_DESTINATION, StageBits::COPY};
    for (uint32_t i = 0; i < bufferUploadDescNum;)
    {
        const uint32_t passBegin = i;
        const uint32_t passEnd = std::min(i + BUFFERS_PER_PASS, bufferUploadDescNum);

        for ( ; i < passEnd; i++)
        {
            const BufferUploadDesc& bufferUploadDesc = bufferUploadDescs[i];

            BufferBarrierDesc& barrier = bufferBarriers[i - passBegin];
            barrier = {};
            barrier.buffer = bufferUploadDesc.buffer;
            barrier.before = isInitialTransition ? bufferUploadDesc.before : state;
            barrier.after = isInitialTransition ? state : bufferUploadDesc.after;
        }

        BarrierGroupDesc barrierGroup = {};
        barrierGroup.buffers = bufferBarriers;
        barrierGroup.bufferNum = uint16_t(passEnd - passBegin);

        for (uint32_t j = 0; j < m_CommandBuffers.size(); j++)
            NRI.CmdBarrier(*m_CommandBuffers[j], barrierGroup);
    }
}
