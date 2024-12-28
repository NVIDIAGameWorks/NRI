#include "SharedMTL.h"

#include "CommandBufferMTL.h"

#include "BufferMTL.h"
#include "CommandBufferMTL.h"
#include "CommandQueueMTL.h"
#include "DescriptorMTL.h"
#include "DescriptorSetMTL.h"
#include "PipelineLayoutMTL.h"
#include "PipelineMTL.h"
#include "PipelineLayoutMTL.h"
#include "TextureMTL.h"

using namespace nri;

CommandBufferMTL::~CommandBufferMTL() {

}


CommandBufferMTL::CommandBufferMTL(DeviceMTL& device): m_Device(device) {
    m_Annotations = [NSMutableArray alloc];
}

void CommandBufferMTL::SetDebugName(const char* name) {
    [m_Handle setLabel:[NSString stringWithUTF8String:name]];
}

Result CommandBufferMTL::Begin(const DescriptorPool* descriptorPool) {
}

Result CommandBufferMTL::End() {
    InsertBarriers();
    EndCurrentEncoders();
}

void CommandBufferMTL::SetPipeline(const Pipeline& pipeline) {
    if (m_CurrentPipeline == (PipelineMTL*)&pipeline)
        return;
    
    PipelineMTL& pipelineImpl = (PipelineMTL&)pipeline;
    m_CurrentPipeline = &pipelineImpl;
    switch(m_CurrentPipeline->GetPipelineType()) {
        case PipelineType::Compute: {
            if(!m_ComputeEncoder) {
                MTLComputePassDescriptor* computePassDescriptor = [MTLComputePassDescriptor computePassDescriptor];
                m_ComputeEncoder = [m_Handle computeCommandEncoderWithDescriptor:computePassDescriptor];
                
                while(m_Annotations.count > 0) {
                    NSObject* obj = m_Annotations[m_Annotations.count - 1];
                    if([obj isKindOfClass: [NSNull class]]) {
                        [m_ComputeEncoder popDebugGroup];
                    } else {
                        [m_ComputeEncoder insertDebugSignpost: (NSString*)obj];
                    }
                    [m_Annotations removeLastObject];
                }
               
            }
            [m_ComputeEncoder setComputePipelineState: m_CurrentPipeline->GetComputePipeline()];
            break;
        }
        case PipelineType::Graphics: {
            if(!m_RendererEncoder) {
                m_RendererEncoder = [m_Handle renderCommandEncoderWithDescriptor: m_renderPassDescriptor];
                
                uint32_t vertexSlot = 0;
                for( uint32_t attr = m_dirtyVertexBufferBits; attr > 0; attr = ( attr >> 1 ), vertexSlot++ )
                    [m_RendererEncoder setVertexBuffer: m_vertexBuffers[vertexSlot].m_Buffer->GetHandle()
                                                offset: m_vertexBuffers[vertexSlot].m_Offset
                                               atIndex: vertexSlot];
                
                while(m_Annotations.count > 0) {
                    NSObject* obj = m_Annotations[m_Annotations.count - 1];
                    if([obj isKindOfClass: [NSNull class]]) {
                        [m_RendererEncoder popDebugGroup];
                    } else {
                        [m_RendererEncoder insertDebugSignpost: (NSString*)obj];
                    }
                    [m_Annotations removeLastObject];
                }
                if(m_numViewports > 0)
                    [m_RendererEncoder setViewports: m_viewports count: m_numViewports];
                if(m_numScissors > 0)
                    [m_RendererEncoder setScissorRects: m_Scissors count: m_numScissors];
                if(m_DirtyBits & CommandBufferDirtyBits::CMD_DIRTY_BLEND_CONSTANT)
                    [m_RendererEncoder setBlendColorRed: m_BlendColor.x
                                                  green: m_BlendColor.y
                                                   blue: m_BlendColor.z
                                                  alpha: m_BlendColor.w];
                if(m_DirtyBits & CommandBufferDirtyBits::CMD_DIRTY_STENCIL)
                    [m_RendererEncoder
                     setStencilFrontReferenceValue: m_StencilFront
                     backReferenceValue: m_StencilBack];
            }
           
            [m_RendererEncoder setRenderPipelineState: m_CurrentPipeline->GetGraphicsPipeline()];
            break;
        }
        case PipelineType::Raytracing: {
            break;
        }
    }
}

void CommandBufferMTL::SetPipelineLayout(const PipelineLayout& pipelineLayout) {
    PipelineLayoutMTL& pipelineLayoutMTL = (PipelineLayoutMTL&)pipelineLayout;
    m_CurrentPipelineLayout = &pipelineLayoutMTL;
}

void CommandBufferMTL::SetDescriptorSet(uint32_t setIndexInPipelineLayout, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    const DescriptorSetMTL& descriptorSetImpl = (DescriptorSetMTL&)descriptorSet;
    auto* layout = m_CurrentPipelineLayout->GetDescriptorSetLayout(setIndexInPipelineLayout);
    const uint32_t space = layout->m_DescriptorSetDesc.registerSpace;
    
    StageBits stageBits = (StageBits)0;
    for(size_t i = 0; i < layout->m_DescriptorSetDesc.rangeNum; i++) {
        if(layout->m_DescriptorSetDesc.ranges[i].shaderStages == StageBits::NONE)
            continue;
        stageBits |= layout->m_DescriptorSetDesc.ranges[i].shaderStages ;
    }
    if(stageBits == StageBits::ALL ||
       stageBits & StageBits::FRAGMENT_SHADER) {
        [m_RendererEncoder setFragmentBuffer: descriptorSetImpl.GetArgumentBuffer()
                                      offset: descriptorSetImpl.GetArugmentBufferOffset()
                                     atIndex: space];
    }
    if(stageBits == StageBits::ALL ||
       stageBits & StageBits::VERTEX_SHADER) {
        [m_RendererEncoder setVertexBuffer: descriptorSetImpl.GetArgumentBuffer()
                                    offset: descriptorSetImpl.GetArugmentBufferOffset()
                                   atIndex: space];
        
    }
    
    if(stageBits == StageBits::ALL ||
       stageBits & StageBits::COMPUTE_SHADER) {
        [m_ComputeEncoder setBuffer: descriptorSetImpl.GetArgumentBuffer()
                                    offset: descriptorSetImpl.GetArugmentBufferOffset()
                                   atIndex: space];
        
    }
    
}

void CommandBufferMTL::SetConstants(uint32_t pushConstantIndex, const void* data, uint32_t size) {
    const struct RootConstantDesc* pDesc = m_CurrentPipelineLayout->GetPushBinding(pushConstantIndex);
    
    
    if (pDesc->shaderStages == StageBits::ALL ||
        pDesc->shaderStages & StageBits::VERTEX_SHADER)
    {
        [m_RendererEncoder setVertexBytes:data length:size atIndex:pDesc->registerIndex];
    }
    
    if (pDesc->shaderStages == StageBits::ALL ||
        pDesc->shaderStages & StageBits::FRAGMENT_SHADER )
    {
        [m_RendererEncoder setFragmentBytes:data length:size atIndex:pDesc->registerIndex];
    }
    
    if (pDesc->shaderStages == StageBits::ALL ||
        pDesc->shaderStages & StageBits::COMPUTE_SHADER )
    {
        [m_ComputeEncoder setBytes:data length:size atIndex:pDesc->registerIndex];
    }
}

void CommandBufferMTL::SetDescriptorPool(const DescriptorPool& descriptorPool) {
    
}


void CommandBufferMTL::InsertBarriers() {
    if(m_RendererEncoder) {
        if (m_barrierFlags & BarrierBits::BARRIER_FLAG_BUFFERS)
        {
            [m_RendererEncoder memoryBarrierWithScope:MTLBarrierScopeBuffers
                                          afterStages:MTLRenderStageFragment
                                         beforeStages:MTLRenderStageVertex];
        }
        
        if (m_barrierFlags & BarrierBits::BARRIER_FLAG_TEXTURES)
        {
            [m_RendererEncoder memoryBarrierWithScope:MTLBarrierScopeTextures
                                          afterStages:MTLRenderStageFragment
                                         beforeStages:MTLRenderStageVertex];
        }
    }
    
    if(m_ComputeEncoder) {
        if (m_barrierFlags & BarrierBits::BARRIER_FLAG_BUFFERS)
        {
            [m_ComputeEncoder memoryBarrierWithScope:MTLBarrierScopeBuffers];
        }
        
        if (m_barrierFlags & BarrierBits::BARRIER_FLAG_TEXTURES)
        {
            [m_ComputeEncoder memoryBarrierWithScope:MTLBarrierScopeTextures];
        }
        
    }
    
    m_barrierFlags = BarrierBits::NONE;
}

void CommandBufferMTL::Barrier(const BarrierGroupDesc& barrierGroupDesc) {
    
    if(barrierGroupDesc.bufferNum) {
        m_barrierFlags |= BarrierBits::BARRIER_FLAG_BUFFERS;
    }
    
    if(barrierGroupDesc.textureNum) {
        m_barrierFlags |= BarrierBits::BARRIER_FLAG_TEXTURES;
    }
//    
//    const size_t totalResources = barrierGroupDesc.bufferNum + barrierGroupDesc.textureNum;
//    Scratch<id<MTLResource>> resourceBarrier = AllocateScratch(m_Device, id<MTLResource>, totalResources);
//    size_t barrierCount = 0;
//    for(size_t i = 0; i < barrierGroupDesc.bufferNum; i++) {
//        const BufferBarrierDesc& in = barrierGroupDesc.buffers[i];
//        const BufferMTL& bufferImpl = *(const BufferMTL*)in.buffer;
//        resourceBarrier[barrierCount++] = bufferImpl.GetHandle();
//    }
//    
//    for(size_t i = 0; i < barrierGroupDesc.textureNum; i++) {
//        const TextureBarrierDesc& in = barrierGroupDesc.textures[i];
//        const TextureMTL& textureImpl = *(const TextureMTL*)in.texture;
//        resourceBarrier[barrierCount++] = textureImpl.GetHandle();
//    }
    
//    [m_RendererEncoder memoryBarrierWithResources: resourceBarrier
//                                            count: barrierCount
//                                      afterStages: MTLRenderStageFragment
//                                     beforeStages: MTLRenderStageVertex];
}
void CommandBufferMTL::BeginRendering(const AttachmentsDesc& attachmentsDesc) {
    m_renderPassDescriptor = [MTLRenderPassDescriptor alloc];
    
    for(uint32_t i = 0; i < attachmentsDesc.colorNum; i++) {
        DescriptorMTL& descriptorMTL = *(DescriptorMTL*)attachmentsDesc.colors[i];
        
        m_renderPassDescriptor.colorAttachments[i].texture = descriptorMTL.GetTextureHandle();
        m_renderPassDescriptor.colorAttachments[i].clearColor = MTLClearColorMake(0, 0, 0, 1);
        m_renderPassDescriptor.colorAttachments[i].loadAction = MTLLoadActionLoad;
        m_renderPassDescriptor.colorAttachments[i].storeAction = MTLStoreActionStore;
    }
    
    if(attachmentsDesc.depthStencil) {
        DescriptorMTL& descriptorMTL = *(DescriptorMTL*)attachmentsDesc.depthStencil;
        m_renderPassDescriptor.depthAttachment.texture = descriptorMTL.GetTextureHandle();
        m_renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionLoad;
        m_renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
    }
}

void CommandBufferMTL::EndRendering() {
    EndCurrentEncoders();
    m_numScissors = 0;
    m_numViewports = 0;
    m_DirtyBits = CommandBufferDirtyBits::NONE;
}

void CommandBufferMTL::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    for(size_t i = 0; i < viewportNum; i++) {
        m_viewports[i].originX = viewports[i].x;
        m_viewports[i].originY = viewports[i].y;
        m_viewports[i].width = viewports[i].width;
        m_viewports[i].height = viewports[i].height;
        m_viewports[i].znear = viewports[i].depthMin;
        m_viewports[i].zfar = viewports[i].depthMax;
    }
    m_numViewports = viewportNum;
    if(m_RendererEncoder && m_numViewports > 0)
        [m_RendererEncoder
         setViewports: m_viewports
         count: m_numViewports];
}


void CommandBufferMTL::SetScissors(const Rect* rects, uint32_t rectNum) {
    for(size_t i = 0; i < rectNum; i++) {
        m_Scissors[i].x = rects[i].x;
        m_Scissors[i].y = rects[i].y;
        m_Scissors[i].width = rects[i].width;
        m_Scissors[i].height = rects[i].height;
    }
    m_numScissors = rectNum;
    if(m_RendererEncoder && m_numScissors > 0)
        [m_RendererEncoder
         setScissorRects: m_Scissors
         count: m_numScissors];
    
}
void CommandBufferMTL::SetDepthBounds(float boundsMin, float boundsMax) {
    //[m_RendererEncoder set]
}
void CommandBufferMTL::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    m_DirtyBits |= CommandBufferDirtyBits::CMD_DIRTY_STENCIL;
    m_StencilFront = frontRef;
    m_StencilBack = backRef;
    if (m_RendererEncoder)
        [m_RendererEncoder
         setStencilFrontReferenceValue: frontRef
         backReferenceValue: backRef];
}

void CommandBufferMTL::SetBlendConstants(const Color32f& color) {
    m_BlendColor = color;
    m_DirtyBits |= CommandBufferDirtyBits::CMD_DIRTY_BLEND_CONSTANT;
    if (m_RendererEncoder)
        [m_RendererEncoder setBlendColorRed: m_BlendColor.x
                                      green: m_BlendColor.y
                                       blue: m_BlendColor.z
                                      alpha: m_BlendColor.w];
    
    
}
void CommandBufferMTL::SetShadingRate(const ShadingRateDesc& shadingRateDesc) {
    m_shadingRateDesc = shadingRateDesc;
    m_DirtyBits |= CommandBufferDirtyBits::CMD_DIRTY_SHADING_RATE;
}

void CommandBufferMTL::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
}

void CommandBufferMTL::EndCurrentEncoders() {
    if(m_RendererEncoder) {
        [m_RendererEncoder endEncoding];
        m_RendererEncoder = nil;
    }
    
    if(m_ComputeEncoder) {
        [m_ComputeEncoder endEncoding];
        m_ComputeEncoder = nil;
    }
    
    if(m_BlitEncoder) {
        [m_BlitEncoder endEncoding];
        m_BlitEncoder = nil;
    }
}

void CommandBufferMTL::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    m_indexBuffer.m_Buffer = &(BufferMTL&)buffer;
    switch(indexType) {
        case IndexType::UINT16:
            m_indexBuffer.m_Type = MTLIndexType::MTLIndexTypeUInt16;
            break;
        default:
        case IndexType::UINT32:
            m_indexBuffer.m_Type = MTLIndexType::MTLIndexTypeUInt32;
            break;
    }
    m_indexBuffer.m_Offset = offset;
}


void CommandBufferMTL::SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    for(size_t i = 0; i < bufferNum; i++) {
        BufferMTL* mtlBuffer = (BufferMTL*)buffers[i];
        
        const size_t slotIndex = i + baseSlot;
        m_vertexBuffers[slotIndex].m_Offset = offsets[i];
        m_vertexBuffers[slotIndex].m_Buffer = mtlBuffer;
        m_dirtyVertexBufferBits |= (1 << slotIndex);
        
        if(m_RendererEncoder)
            [m_RendererEncoder setVertexBuffer: mtlBuffer->GetHandle()
                                    offset: offsets[i]
                                   atIndex: i + baseSlot];
    }
    
}

void CommandBufferMTL::Draw(const DrawDesc& drawDesc) {
    [m_RendererEncoder drawPrimitives: m_CurrentPipeline->GetPrimitiveType()
                          vertexStart:drawDesc.baseVertex
                          vertexCount:drawDesc.vertexNum
                        instanceCount:drawDesc.instanceNum
                         baseInstance: drawDesc.baseInstance];
    
}

void CommandBufferMTL::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    id<MTLBuffer> indexBuffer = m_indexBuffer.m_Buffer->GetHandle();
    [m_RendererEncoder drawIndexedPrimitives: m_CurrentPipeline->GetPrimitiveType()
                              indexCount: drawIndexedDesc.indexNum
                               indexType: m_indexBuffer.m_Type
                             indexBuffer: indexBuffer
                       indexBufferOffset: m_indexBuffer.m_Offset];
}

void CommandBufferMTL::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    InsertBarriers();
    [m_RendererEncoder
         drawPrimitives: m_CurrentPipeline->GetPrimitiveType()
         indirectBuffer:((BufferMTL&)buffer).GetHandle()
         indirectBufferOffset: offset];
}
void CommandBufferMTL::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    InsertBarriers();
    
    id<MTLBuffer> indexBuffer = m_indexBuffer.m_Buffer->GetHandle();
    const BufferMTL& bufferImpl = (const BufferMTL&)buffer;
    [m_RendererEncoder drawIndexedPrimitives: m_CurrentPipeline->GetPrimitiveType()
                              indexCount: drawNum
                               indexType: m_indexBuffer.m_Type
                             indexBuffer: indexBuffer
                       indexBufferOffset: m_indexBuffer.m_Offset
                      ];
}
void CommandBufferMTL::Dispatch(const DispatchDesc& dispatchDesc) {
    InsertBarriers();
    
    MTLSize threadgroupCount = MTLSizeMake(dispatchDesc.x, dispatchDesc.y, dispatchDesc.z);
//    MTLSize threadPerThreadGroup = MTLSizeMake(dispatchDesc.x, dispatchDesc.y, dispatchDesc.z);
//    [m_ComputeEncoder
//     dispatchThreadgroups: threadgroupCount
//     threadsPerThreadgroup: threadPerThreadGroup];
}
void CommandBufferMTL::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    InsertBarriers();
    
    const BufferMTL& bufferImpl = (const BufferMTL&)buffer;
    id<MTLBuffer> dispatchBuffer = bufferImpl.GetHandle();
//    MTLSize threadPerThreadGroup = MTLSizeMake(0, 0, 0);
//    [m_ComputeEncoder
//     dispatchThreadgroupsWithIndirectBuffer: dispatchBuffer
//     indirectBufferOffset: offset
//     threadsPerThreadgroup: threadPerThreadGroup];
}
void CommandBufferMTL::BeginQuery(const QueryPool& queryPool, uint32_t offset) {
    
}
void CommandBufferMTL::EndQuery(const QueryPool& queryPool, uint32_t offset) {
    
}


void CommandBufferMTL::BeginAnnotation(const char* name) {
    if(m_RendererEncoder) {
        [m_RendererEncoder pushDebugGroup:  [NSString stringWithUTF8String:name]];
        return;
    }
    
    if(m_ComputeEncoder) {
        [m_ComputeEncoder pushDebugGroup:  [NSString stringWithUTF8String:name]];
        return;
    }
    
    if(m_BlitEncoder) {
        [m_BlitEncoder pushDebugGroup:  [NSString stringWithUTF8String:name]];
        return;
    }

    [m_Annotations addObject:  [NSString stringWithUTF8String:name]];
    
}
void CommandBufferMTL::EndAnnotation() {
    if(m_RendererEncoder) {
        [m_RendererEncoder popDebugGroup];
        return;
    }
    
    if(m_ComputeEncoder) {
        [m_ComputeEncoder popDebugGroup];
        return;
    }
    
    if(m_BlitEncoder) {
        [m_BlitEncoder popDebugGroup];
        return;
    }
    [m_Annotations addObject: [NSNull null]];
//    m_DirtyBits |= CommandBufferDirtyBits::CMD_DIRTY_END_ANNOTATION;
}
void CommandBufferMTL::ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc) {
    
}
void CommandBufferMTL::ClearStorageTexture(const ClearStorageTextureDesc& clearDesc) {
    
}

void CommandBufferMTL::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    const BufferMTL& src = (const BufferMTL&)srcBuffer;
    const BufferMTL& dst = (const BufferMTL&)dstBuffer;
    
    if(!m_BlitEncoder) {
        EndCurrentEncoders();
        m_BlitEncoder = [m_Handle blitCommandEncoder];
    }
    
    [m_BlitEncoder
     copyFromBuffer:src.GetHandle()
     sourceOffset:srcOffset
     toBuffer: dst.GetHandle()
     destinationOffset:dstOffset
     size:size];
}

void CommandBufferMTL::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    const TextureMTL& src = (const TextureMTL&)srcTexture;
    const TextureMTL& dst = (const TextureMTL&)dstTexture;
    if(!m_BlitEncoder) {
        EndCurrentEncoders();
        m_BlitEncoder = [m_Handle blitCommandEncoder];
    }
    
    bool isWholeResource = !dstRegionDesc && !srcRegionDesc;
    if (isWholeResource) {
        [m_BlitEncoder
         copyFromTexture: src.GetHandle()
         toTexture: dst.GetHandle()];
    } else {
        TextureRegionDesc wholeResource = {};
        if (!srcRegionDesc)
            srcRegionDesc = &wholeResource;
        if (!dstRegionDesc)
            dstRegionDesc = &wholeResource;
        const MTLSize sourceSize = MTLSizeMake(
                                               (srcRegionDesc->width == WHOLE_SIZE) ? src.GetSize(0, srcRegionDesc->mipOffset) : srcRegionDesc->width,
                                               (srcRegionDesc->height == WHOLE_SIZE) ? src.GetSize(1, srcRegionDesc->mipOffset) : srcRegionDesc->height,
                                               (srcRegionDesc->depth == WHOLE_SIZE) ? src.GetSize(2, srcRegionDesc->mipOffset) : srcRegionDesc->depth
                                               );
        
        // Copy to the texture's final subresource.
        [m_BlitEncoder copyFromTexture: src.GetHandle()
                           sourceSlice: srcRegionDesc->layerOffset
                           sourceLevel: srcRegionDesc->mipOffset
                          sourceOrigin: MTLOriginMake(srcRegionDesc->x, srcRegionDesc->y, srcRegionDesc->z)
                            sourceSize: sourceSize
                             toTexture: dst.GetHandle()
                      destinationSlice: dstRegionDesc->layerOffset
                      destinationLevel: dstRegionDesc->mipOffset
                     destinationOrigin: MTLOriginMake(dstRegionDesc->x, dstRegionDesc->y, dstRegionDesc->z)
        ];
    }
}
void CommandBufferMTL::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {
    const BufferMTL& src = (const BufferMTL&)srcBuffer;
    const TextureMTL& dst = (const TextureMTL&)dstTexture;
    
    if(!m_BlitEncoder) {
        EndCurrentEncoders();
        m_BlitEncoder = [m_Handle blitCommandEncoder];
    }
    
    const MTLSize sourceSize = MTLSizeMake(
                                       (dstRegionDesc.width == WHOLE_SIZE) ? dst.GetSize(0, dstRegionDesc.mipOffset) : dstRegionDesc.width,
                                       (dstRegionDesc.height == WHOLE_SIZE) ? dst.GetSize(1, dstRegionDesc.mipOffset) : dstRegionDesc.height,
                                       (dstRegionDesc.depth == WHOLE_SIZE) ? dst.GetSize(2, dstRegionDesc.mipOffset) : dstRegionDesc.depth
                                       );
    [m_BlitEncoder
     copyFromBuffer: src.GetHandle()
     sourceOffset: srcDataLayoutDesc.offset
     sourceBytesPerRow: srcDataLayoutDesc.rowPitch
     sourceBytesPerImage: srcDataLayoutDesc.slicePitch
     sourceSize: sourceSize
     toTexture: dst.GetHandle()
     destinationSlice: dstRegionDesc.layerOffset
     destinationLevel: dstRegionDesc.mipOffset
     destinationOrigin: MTLOriginMake(dstRegionDesc.x, dstRegionDesc.y, dstRegionDesc.z)
     options: MTLBlitOptionNone];
}
void CommandBufferMTL::ReadbackTextureToBuffer(Buffer& dstBuffer, TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {
    const TextureMTL& src = (const TextureMTL&)srcTexture;
    const BufferMTL& dst = (const BufferMTL&)dstBuffer;
    
    if(!m_BlitEncoder) {
        EndCurrentEncoders();
        m_BlitEncoder = [m_Handle blitCommandEncoder];
    }
    
    const MTLSize sourceSize = MTLSizeMake(
                                       (srcRegionDesc.width == WHOLE_SIZE) ? src.GetSize(0, srcRegionDesc.mipOffset) : srcRegionDesc.width,
                                       (srcRegionDesc.height == WHOLE_SIZE) ? src.GetSize(1, srcRegionDesc.mipOffset) : srcRegionDesc.height,
                                       (srcRegionDesc.depth == WHOLE_SIZE) ? src.GetSize(2, srcRegionDesc.mipOffset) : srcRegionDesc.depth
                                       );
    [m_BlitEncoder copyFromTexture: src.GetHandle()
                       sourceSlice: srcRegionDesc.layerOffset
                       sourceLevel: srcRegionDesc.mipOffset
                      sourceOrigin: MTLOriginMake(srcRegionDesc.x, srcRegionDesc.y, srcRegionDesc.z)
                        sourceSize: sourceSize
                          toBuffer: dst.GetHandle()
                 destinationOffset: dstDataLayoutDesc.offset
            destinationBytesPerRow: dstDataLayoutDesc.rowPitch
          destinationBytesPerImage: dstDataLayoutDesc.slicePitch
                           options: MTLBlitOptionNone];
}
void CommandBufferMTL::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {}
void CommandBufferMTL::ResetQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num) {}
void CommandBufferMTL::BuildTopLevelAccelerationStructure(uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset) {}
void CommandBufferMTL::BuildBottomLevelAccelerationStructure(uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset) {}
void CommandBufferMTL::UpdateTopLevelAccelerationStructure(uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset) {}
void CommandBufferMTL::UpdateBottomLevelAccelerationStructure(uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset) {}
void CommandBufferMTL::CopyAccelerationStructure(AccelerationStructure& dst, AccelerationStructure& src, CopyMode copyMode) {}
void CommandBufferMTL::WriteAccelerationStructureSize(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryPoolOffset) {}
void CommandBufferMTL::DispatchRays(const DispatchRaysDesc& dispatchRaysDesc) {}
void CommandBufferMTL::DispatchRaysIndirect(const Buffer& buffer, uint64_t offset) {}
void CommandBufferMTL::DrawMeshTasks(const DrawMeshTasksDesc& drawMeshTasksDesc) {}
void CommandBufferMTL::DrawMeshTasksIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride) {
    
}


void CommandBufferMTL::Create(const struct CommandQueueMTL* queue) {
    m_CommandQueue = queue;
    MTLCommandBufferDescriptor* pDesc = [[MTLCommandBufferDescriptor alloc] init];
    pDesc.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
    m_Handle = [m_CommandQueue->GetHandle() commandBufferWithDescriptor: pDesc];

}

#include "CommandBufferMTL.hpp"

