// © 2021 NVIDIA Corporation

#include "SharedVK.h"

#include "AccelerationStructureVK.h"
#include "BufferVK.h"
#include "CommandBufferVK.h"
#include "CommandQueueVK.h"
#include "DescriptorSetVK.h"
#include "DescriptorVK.h"
#include "PipelineLayoutVK.h"
#include "PipelineVK.h"
#include "QueryPoolVK.h"
#include "TextureVK.h"

#include <math.h>

using namespace nri;

CommandBufferVK::~CommandBufferVK() {
    if (m_CommandPool == VK_NULL_HANDLE)
        return;

    const auto& vk = m_Device.GetDispatchTable();
    vk.FreeCommandBuffers(m_Device, m_CommandPool, 1, &m_Handle);
}

void CommandBufferVK::Create(VkCommandPool commandPool, VkCommandBuffer commandBuffer, CommandQueueType type) {
    m_CommandPool = commandPool;
    m_Handle = commandBuffer;
    m_Type = type;
}

Result CommandBufferVK::Create(const CommandBufferVKDesc& commandBufferDesc) {
    m_CommandPool = VK_NULL_HANDLE;
    m_Handle = (VkCommandBuffer)commandBufferDesc.vkCommandBuffer;
    m_Type = commandBufferDesc.commandQueueType;

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void CommandBufferVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)m_Handle, name);
}

inline Result CommandBufferVK::Begin(const DescriptorPool* descriptorPool) {
    MaybeUnused(descriptorPool);

    VkCommandBufferBeginInfo info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr};

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.BeginCommandBuffer(m_Handle, &info);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkBeginCommandBuffer returned %d", (int32_t)result);

    m_CurrentPipelineBindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
    m_CurrentPipelineLayoutHandle = VK_NULL_HANDLE;
    m_CurrentPipelineLayout = nullptr;
    m_CurrentPipeline = nullptr;

    return Result::SUCCESS;
}

inline Result CommandBufferVK::End() {
    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.EndCommandBuffer(m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkEndCommandBuffer returned %d", (int32_t)result);

    return Result::SUCCESS;
}

inline void CommandBufferVK::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    VkViewport* vkViewports = STACK_ALLOC(VkViewport, viewportNum);
    for (uint32_t i = 0; i < viewportNum; i++) {
        const Viewport& viewport = viewports[i];
        VkViewport& vkViewport = vkViewports[i];
        vkViewport.x = viewport.x;
        vkViewport.y = viewport.y;
        vkViewport.width = viewport.width;
        vkViewport.height = viewport.height;
        vkViewport.minDepth = viewport.depthRangeMin;
        vkViewport.maxDepth = viewport.depthRangeMax;

        // Flip
        vkViewport.y = viewport.height - viewport.y;
        vkViewport.height = -viewport.height;
    }

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetViewport(m_Handle, 0, viewportNum, vkViewports);
}

inline void CommandBufferVK::SetScissors(const Rect* rects, uint32_t rectNum) {
    VkRect2D* vkRects = STACK_ALLOC(VkRect2D, rectNum);
    for (uint32_t i = 0; i < rectNum; i++) {
        const Rect& viewport = rects[i];
        VkRect2D& vkRect = vkRects[i];
        vkRect.offset.x = viewport.x;
        vkRect.offset.y = viewport.y;
        vkRect.extent.width = viewport.width;
        vkRect.extent.height = viewport.height;
    }

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetScissor(m_Handle, 0, rectNum, vkRects);
}

inline void CommandBufferVK::SetDepthBounds(float boundsMin, float boundsMax) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetDepthBounds(m_Handle, boundsMin, boundsMax);
}

inline void CommandBufferVK::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    const auto& vk = m_Device.GetDispatchTable();

    if (frontRef == backRef)
        vk.CmdSetStencilReference(m_Handle, VK_STENCIL_FRONT_AND_BACK, frontRef);
    else {
        vk.CmdSetStencilReference(m_Handle, VK_STENCIL_FACE_FRONT_BIT, frontRef);
        vk.CmdSetStencilReference(m_Handle, VK_STENCIL_FACE_BACK_BIT, backRef);
    }
}

inline void CommandBufferVK::SetSamplePositions(const SamplePosition* positions, Sample_t positionNum, Sample_t sampleNum) {
    VkSampleLocationEXT* locations = STACK_ALLOC(VkSampleLocationEXT, positionNum);
    for (uint32_t i = 0; i < positionNum; i++)
        locations[i] = {(float)(positions[i].x + 8) / 16.0f, (float)(positions[i].y + 8) / 16.0f};

    uint32_t gridDim = (uint32_t)sqrtf((float)positionNum / (float)sampleNum);

    VkSampleLocationsInfoEXT sampleLocationsInfo = {VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT};
    sampleLocationsInfo.sampleLocationsPerPixel = (VkSampleCountFlagBits)sampleNum;
    sampleLocationsInfo.sampleLocationGridSize = {gridDim, gridDim};
    sampleLocationsInfo.sampleLocationsCount = positionNum;
    sampleLocationsInfo.pSampleLocations = locations;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetSampleLocationsEXT(m_Handle, &sampleLocationsInfo);
}

inline void CommandBufferVK::SetBlendConstants(const Color32f& color) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetBlendConstants(m_Handle, &color.x);
}

inline void CommandBufferVK::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    VkClearAttachment* attachments = STACK_ALLOC(VkClearAttachment, clearDescNum);

    for (uint32_t i = 0; i < clearDescNum; i++) {
        const ClearDesc& desc = clearDescs[i];
        VkClearAttachment& attachment = attachments[i];

        switch (desc.attachmentContentType) {
            case AttachmentContentType::COLOR:
                attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                break;
            case AttachmentContentType::DEPTH:
                attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                break;
            case AttachmentContentType::STENCIL:
                attachment.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
                break;
            case AttachmentContentType::DEPTH_STENCIL:
                attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                break;
            default:
                attachment.aspectMask = 0;
                break;
        }

        attachment.colorAttachment = desc.colorAttachmentIndex;
        memcpy(&attachment.clearValue, &desc.value, sizeof(VkClearValue));
    }

    VkClearRect* clearRects = nullptr;
    if (rectNum == 0) {
        clearRects = STACK_ALLOC(VkClearRect, clearDescNum);
        rectNum = clearDescNum;

        for (uint32_t i = 0; i < clearDescNum; i++) {
            VkClearRect& clearRect = clearRects[i];
            clearRect.baseArrayLayer = 0;
            clearRect.layerCount = m_RenderLayerNum;
            clearRect.rect = {{0, 0}, {m_RenderWidth, m_RenderHeight}};
        }
    } else {
        clearRects = STACK_ALLOC(VkClearRect, rectNum);

        for (uint32_t i = 0; i < rectNum; i++) {
            const Rect& rect = rects[i];
            VkClearRect& clearRect = clearRects[i];
            clearRect.baseArrayLayer = 0;
            clearRect.layerCount = 1;
            clearRect.rect = {{rect.x, rect.y}, {rect.width, rect.height}};
        }
    }

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdClearAttachments(m_Handle, clearDescNum, attachments, rectNum, clearRects);
}

inline void CommandBufferVK::ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc) {
    const DescriptorVK& descriptor = *(const DescriptorVK*)clearDesc.storageBuffer;
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdFillBuffer(m_Handle, descriptor.GetBuffer(), 0, VK_WHOLE_SIZE, clearDesc.value);
}

inline void CommandBufferVK::ClearStorageTexture(const ClearStorageTextureDesc& clearDesc) {
    const DescriptorVK& descriptor = *(const DescriptorVK*)clearDesc.storageTexture;
    const VkClearColorValue* value = (const VkClearColorValue*)&clearDesc.value;

    VkImageSubresourceRange range;
    descriptor.GetImageSubresourceRange(range);

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdClearColorImage(m_Handle, descriptor.GetImage(), VK_IMAGE_LAYOUT_GENERAL, value, 1, &range);
}

inline void CommandBufferVK::BeginRendering(const AttachmentsDesc& attachmentsDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    m_RenderLayerNum = deviceDesc.attachmentLayerMaxNum;
    m_RenderWidth = deviceDesc.attachmentMaxDim;
    m_RenderHeight = deviceDesc.attachmentMaxDim;

    VkRenderingAttachmentInfo* colors = nullptr;
    if (attachmentsDesc.colorNum) {
        colors = STACK_ALLOC(VkRenderingAttachmentInfo, attachmentsDesc.colorNum);

        for (uint32_t i = 0; i < attachmentsDesc.colorNum; i++) {
            const DescriptorVK& descriptor = *(DescriptorVK*)attachmentsDesc.colors[i];

            VkRenderingAttachmentInfo& color = colors[i];
            color = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            color.imageView = descriptor.GetImageView();
            color.imageLayout = descriptor.GetImageLayout();
            color.resolveMode = VK_RESOLVE_MODE_NONE;
            color.resolveImageView = VK_NULL_HANDLE;
            color.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            color.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            color.clearValue = {};

            const DescriptorTextureDesc& desc = descriptor.GetTextureDesc();
            Dim_t w = desc.texture->GetSize(0, desc.mipOffset);
            Dim_t h = desc.texture->GetSize(1, desc.mipOffset);

            m_RenderLayerNum = std::min(m_RenderLayerNum, desc.arraySize);
            m_RenderWidth = std::min(m_RenderWidth, w);
            m_RenderHeight = std::min(m_RenderHeight, h);
        }
    }

    VkRenderingAttachmentInfo depthStencil = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    if (attachmentsDesc.depthStencil) {
        const DescriptorVK& descriptor = *(DescriptorVK*)attachmentsDesc.depthStencil;

        depthStencil.imageView = descriptor.GetImageView();
        depthStencil.imageLayout = descriptor.GetImageLayout();
        depthStencil.resolveMode = VK_RESOLVE_MODE_NONE;
        depthStencil.resolveImageView = VK_NULL_HANDLE;
        depthStencil.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthStencil.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depthStencil.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthStencil.clearValue = {};

        const DescriptorTextureDesc& desc = descriptor.GetTextureDesc();
        Dim_t w = desc.texture->GetSize(0, desc.mipOffset);
        Dim_t h = desc.texture->GetSize(1, desc.mipOffset);

        m_RenderLayerNum = std::min(m_RenderLayerNum, desc.arraySize);
        m_RenderWidth = std::min(m_RenderWidth, w);
        m_RenderHeight = std::min(m_RenderHeight, h);
    }

    // TODO: matches D3D behavior?
    bool hasAttachment = attachmentsDesc.depthStencil || attachmentsDesc.colors;
    if (!hasAttachment) {
        m_RenderLayerNum = 0;
        m_RenderWidth = 0;
        m_RenderHeight = 0;
    }

    VkRenderingInfo renderingInfo = {
        VK_STRUCTURE_TYPE_RENDERING_INFO,
        nullptr,
        0,
        {{0, 0}, {m_RenderWidth, m_RenderHeight}},
        m_RenderLayerNum,
        0,
        attachmentsDesc.colorNum,
        colors,
        attachmentsDesc.depthStencil ? &depthStencil : nullptr,
        attachmentsDesc.depthStencil ? &depthStencil : nullptr,
    };

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBeginRendering(m_Handle, &renderingInfo);
}

inline void CommandBufferVK::EndRendering() {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdEndRendering(m_Handle);
}

inline void CommandBufferVK::SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    VkBuffer* bufferHandles = STACK_ALLOC(VkBuffer, bufferNum);

    for (uint32_t i = 0; i < bufferNum; i++)
        bufferHandles[i] = GetVulkanHandle<VkBuffer, BufferVK>(buffers[i]);

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBindVertexBuffers(m_Handle, baseSlot, bufferNum, bufferHandles, offsets);
}

inline void CommandBufferVK::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    const VkBuffer bufferHandle = GetVulkanHandle<VkBuffer, BufferVK>(&buffer);
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBindIndexBuffer(m_Handle, bufferHandle, offset, GetIndexType(indexType));
}

inline void CommandBufferVK::SetPipelineLayout(const PipelineLayout& pipelineLayout) {
    const PipelineLayoutVK& pipelineLayoutVK = (const PipelineLayoutVK&)pipelineLayout;

    m_CurrentPipelineLayout = &pipelineLayoutVK;
    m_CurrentPipelineLayoutHandle = pipelineLayoutVK;
    m_CurrentPipelineBindPoint = pipelineLayoutVK.GetPipelineBindPoint();
}

inline void CommandBufferVK::SetPipeline(const Pipeline& pipeline) {
    if (m_CurrentPipeline == (PipelineVK*)&pipeline)
        return;

    const PipelineVK& pipelineImpl = (const PipelineVK&)pipeline;
    m_CurrentPipeline = &pipelineImpl;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBindPipeline(m_Handle, pipelineImpl.GetBindPoint(), pipelineImpl);
}

inline void CommandBufferVK::SetDescriptorPool(const DescriptorPool& descriptorPool) {
    MaybeUnused(descriptorPool);
}

inline void CommandBufferVK::SetDescriptorSet(uint32_t setIndexInPipelineLayout, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    const DescriptorSetVK& descriptorSetImpl = (DescriptorSetVK&)descriptorSet;
    VkDescriptorSet vkDescriptorSet = descriptorSetImpl.GetHandle();

    uint32_t space = m_CurrentPipelineLayout->GetDescriptorSetSpace(setIndexInPipelineLayout);
    uint32_t dynamicOffsetNum = descriptorSetImpl.GetDynamicConstantBufferNum();

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBindDescriptorSets(m_Handle, m_CurrentPipelineBindPoint, m_CurrentPipelineLayoutHandle, space, 1, &vkDescriptorSet, dynamicOffsetNum, dynamicConstantBufferOffsets);
}

inline void CommandBufferVK::SetConstants(uint32_t pushConstantIndex, const void* data, uint32_t size) {
    const auto& bindingInfo = m_CurrentPipelineLayout->GetRuntimeBindingInfo();
    const PushConstantRangeBindingDesc& desc = bindingInfo.pushConstantBindings[pushConstantIndex];

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdPushConstants(m_Handle, m_CurrentPipelineLayoutHandle, desc.flags, desc.offset, size, data);
}

inline void CommandBufferVK::Draw(const DrawDesc& drawDesc) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDraw(m_Handle, drawDesc.vertexNum, drawDesc.instanceNum, drawDesc.baseVertex, drawDesc.baseInstance);
}

inline void CommandBufferVK::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDrawIndexed(m_Handle, drawIndexedDesc.indexNum, drawIndexedDesc.instanceNum, drawIndexedDesc.baseIndex, drawIndexedDesc.baseVertex, drawIndexedDesc.baseInstance);
}

inline void CommandBufferVK::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const VkBuffer bufferHandle = GetVulkanHandle<VkBuffer, BufferVK>(&buffer);
    const auto& vk = m_Device.GetDispatchTable();

    if (countBuffer) {
        const VkBuffer countBufferHandle = GetVulkanHandle<VkBuffer, BufferVK>(countBuffer);
        vk.CmdDrawIndirectCount(m_Handle, bufferHandle, offset, countBufferHandle, countBufferOffset, drawNum, (uint32_t)stride);
    }
    else
        vk.CmdDrawIndirect(m_Handle, bufferHandle, offset, drawNum, (uint32_t)stride);
}

inline void CommandBufferVK::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const VkBuffer bufferHandle = GetVulkanHandle<VkBuffer, BufferVK>(&buffer);
    const auto& vk = m_Device.GetDispatchTable();

    if (countBuffer) {
        const VkBuffer countBufferHandle = GetVulkanHandle<VkBuffer, BufferVK>(countBuffer);
        vk.CmdDrawIndexedIndirectCount(m_Handle, bufferHandle, offset, countBufferHandle, countBufferOffset, drawNum, (uint32_t)stride);
    }
    else
        vk.CmdDrawIndexedIndirect(m_Handle, bufferHandle, offset, drawNum, (uint32_t)stride);
}

inline void CommandBufferVK::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    const BufferVK& srcBufferImpl = (const BufferVK&)srcBuffer;
    const BufferVK& dstBufferImpl = (const BufferVK&)dstBuffer;

    const VkBufferCopy region = {srcOffset, dstOffset, size == WHOLE_SIZE ? srcBufferImpl.GetDesc().size : size};

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyBuffer(m_Handle, srcBufferImpl.GetHandle(), dstBufferImpl.GetHandle(), 1, &region);
}

inline void CommandBufferVK::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    const TextureVK& srcTextureImpl = (const TextureVK&)srcTexture;
    const TextureVK& dstTextureImpl = (const TextureVK&)dstTexture;

    if (srcRegionDesc == nullptr && dstRegionDesc == nullptr) {
        CopyWholeTexture(dstTextureImpl, srcTextureImpl);
        return;
    }

    VkImageCopy region;

    if (srcRegionDesc != nullptr) {
        region.srcSubresource = {srcTextureImpl.GetImageAspectFlags(), srcRegionDesc->mipOffset, srcRegionDesc->arrayOffset, 1};

        region.srcOffset = {(int32_t)srcRegionDesc->x, (int32_t)srcRegionDesc->y, (int32_t)srcRegionDesc->z};

        region.extent = {
            (srcRegionDesc->width == WHOLE_SIZE) ? srcTextureImpl.GetSize(0, srcRegionDesc->mipOffset) : srcRegionDesc->width,
            (srcRegionDesc->height == WHOLE_SIZE) ? srcTextureImpl.GetSize(1, srcRegionDesc->mipOffset) : srcRegionDesc->height,
            (srcRegionDesc->depth == WHOLE_SIZE) ? srcTextureImpl.GetSize(2, srcRegionDesc->mipOffset) : srcRegionDesc->depth,
        };
    } else {
        region.srcSubresource = {srcTextureImpl.GetImageAspectFlags(), 0, 0, 1};

        region.srcOffset = {};
        region.extent = srcTextureImpl.GetExtent();
    }

    if (dstRegionDesc != nullptr) {
        region.dstSubresource = {dstTextureImpl.GetImageAspectFlags(), dstRegionDesc->mipOffset, dstRegionDesc->arrayOffset, 1};

        region.dstOffset = {(int32_t)dstRegionDesc->x, (int32_t)dstRegionDesc->y, (int32_t)dstRegionDesc->z};
    } else {
        region.dstSubresource = {dstTextureImpl.GetImageAspectFlags(), 0, 0, 1};

        region.dstOffset = {};
    }

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyImage(m_Handle, srcTextureImpl.GetHandle(), IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstTextureImpl.GetHandle(), IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

inline void CommandBufferVK::UploadBufferToTexture(
    Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {
    const BufferVK& srcBufferImpl = (const BufferVK&)srcBuffer;
    const TextureVK& dstTextureImpl = (const TextureVK&)dstTexture;

    const FormatProps& formatProps = GetFormatProps(dstTextureImpl.GetDesc().format);
    const uint32_t rowBlockNum = srcDataLayoutDesc.rowPitch / formatProps.stride;
    const uint32_t bufferRowLength = rowBlockNum * formatProps.blockWidth;

    const uint32_t sliceRowNum = srcDataLayoutDesc.slicePitch / srcDataLayoutDesc.rowPitch;
    const uint32_t bufferImageHeight = sliceRowNum * formatProps.blockWidth;

    const VkBufferImageCopy region = {
        srcDataLayoutDesc.offset,
        bufferRowLength,
        bufferImageHeight,
        VkImageSubresourceLayers{dstTextureImpl.GetImageAspectFlags(), dstRegionDesc.mipOffset, dstRegionDesc.arrayOffset, 1},
        VkOffset3D{dstRegionDesc.x, dstRegionDesc.y, dstRegionDesc.z},
        VkExtent3D{
            (dstRegionDesc.width == WHOLE_SIZE) ? dstTextureImpl.GetSize(0, dstRegionDesc.mipOffset) : dstRegionDesc.width,
            (dstRegionDesc.height == WHOLE_SIZE) ? dstTextureImpl.GetSize(1, dstRegionDesc.mipOffset) : dstRegionDesc.height,
            (dstRegionDesc.depth == WHOLE_SIZE) ? dstTextureImpl.GetSize(2, dstRegionDesc.mipOffset) : dstRegionDesc.depth,
        },
    };

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyBufferToImage(m_Handle, srcBufferImpl.GetHandle(), dstTextureImpl.GetHandle(), IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

inline void CommandBufferVK::ReadbackTextureToBuffer(
    Buffer& dstBuffer, TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {
    const TextureVK& srcTextureImpl = (const TextureVK&)srcTexture;
    const BufferVK& dstBufferImpl = (const BufferVK&)dstBuffer;

    const FormatProps& formatProps = GetFormatProps(srcTextureImpl.GetDesc().format);
    const uint32_t rowBlockNum = dstDataLayoutDesc.rowPitch / formatProps.stride;
    const uint32_t bufferRowLength = rowBlockNum * formatProps.blockWidth;

    const uint32_t sliceRowNum = dstDataLayoutDesc.slicePitch / dstDataLayoutDesc.rowPitch;
    const uint32_t bufferImageHeight = sliceRowNum * formatProps.blockWidth;

    const VkBufferImageCopy region = {
        dstDataLayoutDesc.offset,
        bufferRowLength,
        bufferImageHeight,
        VkImageSubresourceLayers{srcTextureImpl.GetImageAspectFlags(), srcRegionDesc.mipOffset, srcRegionDesc.arrayOffset, 1},
        VkOffset3D{srcRegionDesc.x, srcRegionDesc.y, srcRegionDesc.z},
        VkExtent3D{
            (srcRegionDesc.width == WHOLE_SIZE) ? srcTextureImpl.GetSize(0, srcRegionDesc.mipOffset) : srcRegionDesc.width,
            (srcRegionDesc.height == WHOLE_SIZE) ? srcTextureImpl.GetSize(1, srcRegionDesc.mipOffset) : srcRegionDesc.height,
            (srcRegionDesc.depth == WHOLE_SIZE) ? srcTextureImpl.GetSize(2, srcRegionDesc.mipOffset) : srcRegionDesc.depth,
        },
    };

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyImageToBuffer(m_Handle, srcTextureImpl.GetHandle(), IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstBufferImpl.GetHandle(), 1, &region);
}

inline void CommandBufferVK::Dispatch(const DispatchDesc& dispatchDesc) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDispatch(m_Handle, dispatchDesc.x, dispatchDesc.y, dispatchDesc.z);
}

inline void CommandBufferVK::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    static_assert(sizeof(DispatchDesc) == sizeof(VkDispatchIndirectCommand));

    const BufferVK& bufferImpl = (const BufferVK&)buffer;
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDispatchIndirect(m_Handle, bufferImpl.GetHandle(), offset);
}

static inline VkAccessFlags2 GetAccessFlags(AccessBits accessBits) {
    VkAccessFlags2 flags = 0;

    if (accessBits & AccessBits::VERTEX_BUFFER)
        flags |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;

    if (accessBits & AccessBits::INDEX_BUFFER)
        flags |= VK_ACCESS_2_INDEX_READ_BIT;

    if (accessBits & AccessBits::CONSTANT_BUFFER)
        flags |= VK_ACCESS_2_UNIFORM_READ_BIT;

    if (accessBits & AccessBits::ARGUMENT_BUFFER)
        flags |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;

    if (accessBits & AccessBits::SHADER_RESOURCE)
        flags |= VK_ACCESS_2_SHADER_READ_BIT;

    if (accessBits & AccessBits::SHADER_RESOURCE_STORAGE)
        flags |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;

    if (accessBits & AccessBits::COLOR_ATTACHMENT)
        flags |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

    if (accessBits & AccessBits::DEPTH_STENCIL_WRITE)
        flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    if (accessBits & AccessBits::DEPTH_STENCIL_READ)
        flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

    if (accessBits & AccessBits::COPY_SOURCE)
        flags |= VK_ACCESS_2_TRANSFER_READ_BIT;

    if (accessBits & AccessBits::COPY_DESTINATION)
        flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;

    if (accessBits & AccessBits::ACCELERATION_STRUCTURE_READ)
        flags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;

    if (accessBits & AccessBits::ACCELERATION_STRUCTURE_WRITE)
        flags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

    if (accessBits & AccessBits::SHADING_RATE)
        flags |= VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;

    return flags;
}

inline void CommandBufferVK::Barrier(const BarrierGroupDesc& barrierGroupDesc) {
    // Global
    VkMemoryBarrier2* memoryBarriers = STACK_ALLOC(VkMemoryBarrier2, barrierGroupDesc.globalNum);
    for (uint16_t i = 0; i < barrierGroupDesc.globalNum; i++) {
        const GlobalBarrierDesc& in = barrierGroupDesc.globals[i];

        VkMemoryBarrier2& out = memoryBarriers[i];
        out = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
        out.srcStageMask = GetPipelineStageFlags(in.before.stages);
        out.srcAccessMask = GetAccessFlags(in.before.access);
        out.dstStageMask = GetPipelineStageFlags(in.after.stages);
        out.dstAccessMask = GetAccessFlags(in.after.access);
    }

    // Buffer
    VkBufferMemoryBarrier2* bufferBarriers = STACK_ALLOC(VkBufferMemoryBarrier2, barrierGroupDesc.bufferNum);
    for (uint16_t i = 0; i < barrierGroupDesc.bufferNum; i++) {
        const BufferBarrierDesc& in = barrierGroupDesc.buffers[i];
        const BufferVK& bufferImpl = *(const BufferVK*)in.buffer;

        VkBufferMemoryBarrier2& out = bufferBarriers[i];
        out = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
        out.srcStageMask = GetPipelineStageFlags(in.before.stages);
        out.srcAccessMask = GetAccessFlags(in.before.access);
        out.dstStageMask = GetPipelineStageFlags(in.after.stages);
        out.dstAccessMask = GetAccessFlags(in.after.access);
        out.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        out.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        out.buffer = bufferImpl.GetHandle();
        out.offset = 0;
        out.size = VK_WHOLE_SIZE;
    }

    // Texture
    VkImageMemoryBarrier2* textureBarriers = STACK_ALLOC(VkImageMemoryBarrier2, barrierGroupDesc.textureNum);
    for (uint16_t i = 0; i < barrierGroupDesc.textureNum; i++) {
        const TextureBarrierDesc& in = barrierGroupDesc.textures[i];
        const TextureVK& textureImpl = *(const TextureVK*)in.texture;

        VkImageMemoryBarrier2& out = textureBarriers[i];
        out = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        out.srcStageMask = GetPipelineStageFlags(in.before.stages);
        out.srcAccessMask = in.before.layout == Layout::PRESENT ? VK_ACCESS_2_MEMORY_READ_BIT : GetAccessFlags(in.before.access);
        out.dstStageMask = GetPipelineStageFlags(in.after.stages);
        out.dstAccessMask = in.after.layout == Layout::PRESENT ? VK_ACCESS_2_MEMORY_READ_BIT : GetAccessFlags(in.after.access);
        out.oldLayout = GetImageLayout(in.before.layout);
        out.newLayout = GetImageLayout(in.after.layout);
        out.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        out.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        out.image = textureImpl.GetHandle();
        out.subresourceRange = {
            textureImpl.GetImageAspectFlags(),
            in.mipOffset,
            (in.mipNum == REMAINING_MIP_LEVELS) ? VK_REMAINING_MIP_LEVELS : in.mipNum,
            in.arrayOffset,
            (in.arraySize == REMAINING_ARRAY_LAYERS) ? VK_REMAINING_ARRAY_LAYERS : in.arraySize,
        };
    }

    // Submit
    VkDependencyInfo dependencyInfo = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependencyInfo.memoryBarrierCount = barrierGroupDesc.globalNum;
    dependencyInfo.pMemoryBarriers = memoryBarriers;
    dependencyInfo.bufferMemoryBarrierCount = barrierGroupDesc.bufferNum;
    dependencyInfo.pBufferMemoryBarriers = bufferBarriers;
    dependencyInfo.imageMemoryBarrierCount = barrierGroupDesc.textureNum;
    dependencyInfo.pImageMemoryBarriers = textureBarriers;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdPipelineBarrier2(m_Handle, &dependencyInfo);
}

inline void CommandBufferVK::BeginQuery(const QueryPool& queryPool, uint32_t offset) {
    const QueryPoolVK& queryPoolImpl = (const QueryPoolVK&)queryPool;
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBeginQuery(m_Handle, queryPoolImpl.GetHandle(), offset, (VkQueryControlFlagBits)0);
}

inline void CommandBufferVK::EndQuery(const QueryPool& queryPool, uint32_t offset) {
    const QueryPoolVK& queryPoolImpl = (const QueryPoolVK&)queryPool;
    const auto& vk = m_Device.GetDispatchTable();

    if (queryPoolImpl.GetType() == VK_QUERY_TYPE_TIMESTAMP)
        vk.CmdWriteTimestamp(m_Handle, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPoolImpl.GetHandle(), offset);
    else
        vk.CmdEndQuery(m_Handle, queryPoolImpl.GetHandle(), offset);
}

inline void CommandBufferVK::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    const QueryPoolVK& queryPoolImpl = (const QueryPoolVK&)queryPool;
    const BufferVK& bufferImpl = (const BufferVK&)dstBuffer;

    // TODO: wait is questionable here, but it's needed to ensure that CopyQueries copies to the destination buffer "complete" values (perf seems unaffected)
    VkQueryResultFlags flags = VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyQueryPoolResults(m_Handle, queryPoolImpl.GetHandle(), offset, num, bufferImpl.GetHandle(), dstOffset, queryPoolImpl.GetQuerySize(), flags);
}

inline void CommandBufferVK::ResetQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num) {
    const QueryPoolVK& queryPoolImpl = (const QueryPoolVK&)queryPool;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdResetQueryPool(m_Handle, queryPoolImpl.GetHandle(), offset, num);
}

inline void CommandBufferVK::BeginAnnotation(const char* name) {
    const auto& vk = m_Device.GetDispatchTable();
    if (vk.CmdBeginDebugUtilsLabelEXT == nullptr)
        return;

    VkDebugUtilsLabelEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    info.pLabelName = name;

    vk.CmdBeginDebugUtilsLabelEXT(m_Handle, &info);
}

inline void CommandBufferVK::EndAnnotation() {
    const auto& vk = m_Device.GetDispatchTable();
    if (vk.CmdEndDebugUtilsLabelEXT == nullptr)
        return;

    vk.CmdEndDebugUtilsLabelEXT(m_Handle);
}

inline void CommandBufferVK::CopyWholeTexture(const TextureVK& dstTexture, const TextureVK& srcTexture) {
    const TextureDesc& dstTextureDesc = dstTexture.GetDesc();
    const TextureDesc& srcTextureDesc = srcTexture.GetDesc();

    VkImageCopy* regions = STACK_ALLOC(VkImageCopy, dstTextureDesc.mipNum);

    for (Mip_t i = 0; i < dstTextureDesc.mipNum; i++) {
        regions[i].srcSubresource = {srcTexture.GetImageAspectFlags(), i, 0, srcTextureDesc.arraySize};

        regions[i].dstSubresource = {dstTexture.GetImageAspectFlags(), i, 0, dstTextureDesc.arraySize};

        regions[i].dstOffset = {};
        regions[i].srcOffset = {};
        regions[i].extent = dstTexture.GetExtent();
    }

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyImage(m_Handle, srcTexture.GetHandle(), IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstTexture.GetHandle(), IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstTextureDesc.mipNum, regions);
}

inline void CommandBufferVK::BuildTopLevelAccelerationStructure(
    uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset) {
    const VkAccelerationStructureKHR dstASHandle = ((const AccelerationStructureVK&)dst).GetHandle();
    const VkDeviceAddress scratchAddress = ((BufferVK&)scratch).GetDeviceAddress() + scratchOffset;
    const VkDeviceAddress bufferAddress = ((BufferVK&)buffer).GetDeviceAddress() + bufferOffset;

    VkAccelerationStructureGeometryKHR geometry = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.data.deviceAddress = bufferAddress;

    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildGeometryInfo.flags = GetAccelerationStructureBuildFlags(flags);
    buildGeometryInfo.dstAccelerationStructure = dstASHandle;
    buildGeometryInfo.geometryCount = 1;
    buildGeometryInfo.pGeometries = &geometry;
    buildGeometryInfo.scratchData.deviceAddress = scratchAddress;

    VkAccelerationStructureBuildRangeInfoKHR range = {};
    range.primitiveCount = instanceNum;

    const VkAccelerationStructureBuildRangeInfoKHR* rangeArrays[1] = {&range};

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBuildAccelerationStructuresKHR(m_Handle, 1, &buildGeometryInfo, rangeArrays);
}

inline void CommandBufferVK::BuildBottomLevelAccelerationStructure(
    uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset) {
    const VkAccelerationStructureKHR dstASHandle = ((const AccelerationStructureVK&)dst).GetHandle();
    const VkDeviceAddress scratchAddress = ((BufferVK&)scratch).GetDeviceAddress() + scratchOffset;

    VkAccelerationStructureGeometryKHR* geometries = ALLOCATE_SCRATCH(m_Device, VkAccelerationStructureGeometryKHR, geometryObjectNum);
    VkAccelerationStructureBuildRangeInfoKHR* ranges = ALLOCATE_SCRATCH(m_Device, VkAccelerationStructureBuildRangeInfoKHR, geometryObjectNum);

    ConvertGeometryObjectsVK(geometries, ranges, geometryObjects, geometryObjectNum);

    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildGeometryInfo.flags = GetAccelerationStructureBuildFlags(flags);
    buildGeometryInfo.dstAccelerationStructure = dstASHandle;
    buildGeometryInfo.geometryCount = geometryObjectNum;
    buildGeometryInfo.pGeometries = geometries;
    buildGeometryInfo.scratchData.deviceAddress = scratchAddress;

    const VkAccelerationStructureBuildRangeInfoKHR* rangeArrays[1] = {ranges};

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBuildAccelerationStructuresKHR(m_Handle, 1, &buildGeometryInfo, rangeArrays);

    FREE_SCRATCH(m_Device, ranges, geometryObjectNum);
    FREE_SCRATCH(m_Device, geometries, geometryObjectNum);
}

inline void CommandBufferVK::UpdateTopLevelAccelerationStructure(uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags,
    AccelerationStructure& dst, AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset) {
    const VkAccelerationStructureKHR srcASHandle = ((const AccelerationStructureVK&)src).GetHandle();
    const VkAccelerationStructureKHR dstASHandle = ((const AccelerationStructureVK&)dst).GetHandle();
    const VkDeviceAddress scratchAddress = ((BufferVK&)scratch).GetDeviceAddress() + scratchOffset;
    const VkDeviceAddress bufferAddress = ((BufferVK&)buffer).GetDeviceAddress() + bufferOffset;

    VkAccelerationStructureGeometryKHR geometry = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.data.deviceAddress = bufferAddress;

    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
    buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildGeometryInfo.flags = GetAccelerationStructureBuildFlags(flags);
    buildGeometryInfo.srcAccelerationStructure = srcASHandle;
    buildGeometryInfo.dstAccelerationStructure = dstASHandle;
    buildGeometryInfo.geometryCount = 1;
    buildGeometryInfo.pGeometries = &geometry;
    buildGeometryInfo.scratchData.deviceAddress = scratchAddress;

    VkAccelerationStructureBuildRangeInfoKHR range = {};
    range.primitiveCount = instanceNum;

    const VkAccelerationStructureBuildRangeInfoKHR* rangeArrays[1] = {&range};

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBuildAccelerationStructuresKHR(m_Handle, 1, &buildGeometryInfo, rangeArrays);
}

inline void CommandBufferVK::UpdateBottomLevelAccelerationStructure(uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags,
    AccelerationStructure& dst, AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset) {
    const VkAccelerationStructureKHR srcASHandle = ((const AccelerationStructureVK&)src).GetHandle();
    const VkAccelerationStructureKHR dstASHandle = ((const AccelerationStructureVK&)dst).GetHandle();
    const VkDeviceAddress scratchAddress = ((BufferVK&)scratch).GetDeviceAddress() + scratchOffset;

    VkAccelerationStructureGeometryKHR* geometries = ALLOCATE_SCRATCH(m_Device, VkAccelerationStructureGeometryKHR, geometryObjectNum);
    VkAccelerationStructureBuildRangeInfoKHR* ranges = ALLOCATE_SCRATCH(m_Device, VkAccelerationStructureBuildRangeInfoKHR, geometryObjectNum);

    ConvertGeometryObjectsVK(geometries, ranges, geometryObjects, geometryObjectNum);

    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
    buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildGeometryInfo.flags = GetAccelerationStructureBuildFlags(flags);
    buildGeometryInfo.srcAccelerationStructure = srcASHandle;
    buildGeometryInfo.dstAccelerationStructure = dstASHandle;
    buildGeometryInfo.geometryCount = geometryObjectNum;
    buildGeometryInfo.pGeometries = geometries;
    buildGeometryInfo.scratchData.deviceAddress = scratchAddress;

    const VkAccelerationStructureBuildRangeInfoKHR* rangeArrays[1] = {ranges};

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBuildAccelerationStructuresKHR(m_Handle, 1, &buildGeometryInfo, rangeArrays);

    FREE_SCRATCH(m_Device, ranges, geometryObjectNum);
    FREE_SCRATCH(m_Device, geometries, geometryObjectNum);
}

inline void CommandBufferVK::CopyAccelerationStructure(AccelerationStructure& dst, AccelerationStructure& src, CopyMode copyMode) {
    const VkAccelerationStructureKHR dstASHandle = ((const AccelerationStructureVK&)dst).GetHandle();
    const VkAccelerationStructureKHR srcASHandle = ((const AccelerationStructureVK&)src).GetHandle();

    VkCopyAccelerationStructureInfoKHR info = {VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR};
    info.src = srcASHandle;
    info.dst = dstASHandle;
    info.mode = GetCopyMode(copyMode);

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyAccelerationStructureKHR(m_Handle, &info);
}

inline void CommandBufferVK::WriteAccelerationStructureSize(
    const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    VkAccelerationStructureKHR* ASes = ALLOCATE_SCRATCH(m_Device, VkAccelerationStructureKHR, accelerationStructureNum);

    for (uint32_t i = 0; i < accelerationStructureNum; i++)
        ASes[i] = ((const AccelerationStructureVK*)accelerationStructures[i])->GetHandle();

    const VkQueryPool queryPoolHandle = ((const QueryPoolVK&)queryPool).GetHandle();

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdWriteAccelerationStructuresPropertiesKHR(
        m_Handle, accelerationStructureNum, ASes, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPoolHandle, queryPoolOffset);

    FREE_SCRATCH(m_Device, ASes, accelerationStructureNum);
}

inline void CommandBufferVK::DispatchRays(const DispatchRaysDesc& dispatchRaysDesc) {
    VkStridedDeviceAddressRegionKHR raygen = {};
    raygen.deviceAddress = GetBufferDeviceAddress(dispatchRaysDesc.raygenShader.buffer) + dispatchRaysDesc.raygenShader.offset;
    raygen.size = dispatchRaysDesc.raygenShader.size;
    raygen.stride = dispatchRaysDesc.raygenShader.stride;

    VkStridedDeviceAddressRegionKHR miss = {};
    miss.deviceAddress = GetBufferDeviceAddress(dispatchRaysDesc.missShaders.buffer) + dispatchRaysDesc.missShaders.offset;
    miss.size = dispatchRaysDesc.missShaders.size;
    miss.stride = dispatchRaysDesc.missShaders.stride;

    VkStridedDeviceAddressRegionKHR hit = {};
    hit.deviceAddress = GetBufferDeviceAddress(dispatchRaysDesc.hitShaderGroups.buffer) + dispatchRaysDesc.hitShaderGroups.offset;
    hit.size = dispatchRaysDesc.hitShaderGroups.size;
    hit.stride = dispatchRaysDesc.hitShaderGroups.stride;

    VkStridedDeviceAddressRegionKHR callable = {};
    callable.deviceAddress = GetBufferDeviceAddress(dispatchRaysDesc.callableShaders.buffer) + dispatchRaysDesc.callableShaders.offset;
    callable.size = dispatchRaysDesc.callableShaders.size;
    callable.stride = dispatchRaysDesc.callableShaders.stride;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdTraceRaysKHR(m_Handle, &raygen, &miss, &hit, &callable, dispatchRaysDesc.x, dispatchRaysDesc.y, dispatchRaysDesc.z);
}

inline void CommandBufferVK::DispatchRaysIndirect(const Buffer& buffer, uint64_t offset) {
    static_assert(sizeof(DispatchRaysIndirectDesc) == sizeof(VkTraceRaysIndirectCommand2KHR));

    uint64_t deviceAddress = GetBufferDeviceAddress(&buffer) + offset;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdTraceRaysIndirect2KHR(m_Handle, deviceAddress);
}

inline void CommandBufferVK::DrawMeshTasks(const DrawMeshTasksDesc& drawMeshTasksDesc) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDrawMeshTasksEXT(m_Handle, drawMeshTasksDesc.x, drawMeshTasksDesc.y, drawMeshTasksDesc.z);
}

inline void CommandBufferVK::DrawMeshTasksIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride) {
    static_assert(sizeof(DrawMeshTasksDesc) == sizeof(VkDrawMeshTasksIndirectCommandEXT));

    const BufferVK& bufferImpl = (const BufferVK&)buffer;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDrawMeshTasksIndirectEXT(m_Handle, bufferImpl.GetHandle(), offset, drawNum, stride);
}

#include "CommandBufferVK.hpp"