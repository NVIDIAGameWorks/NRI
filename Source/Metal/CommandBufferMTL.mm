#include "SharedMTL.h"

#include "CommandBufferMTL.h"

#include "BufferMTL.h"
#include "CommandBufferMTL.h"
#include "CommandQueueMTL.h"
#include "DescriptorMTL.h"
#include "PipelineLayoutMTL.h"
#include "PipelineMTL.h"
#include "TextureMTL.h"

#include <math.h>

using namespace nri;

CommandBufferMTL::~CommandBufferMTL() {

}

void CommandBufferMTL::SetDebugName(const char* name) {
    [m_Handle setLabel:[NSString stringWithUTF8String:name]];
}

Result CommandBufferMTL::Begin(const DescriptorPool* descriptorPool) {
    [m_Handle computeCommandEncoderWithDescriptor: NULL];
}

Result CommandBufferMTL::End() {
    m_ComputeEncoder = nil;
    m_RendererEncoder = nil;
    
}

void CommandBufferMTL::SetPipeline(const Pipeline& pipeline) {
    if (m_CurrentPipeline == (PipelineMTL*)&pipeline)
        return;
    PipelineMTL& pipelineImpl = (PipelineMTL&)pipeline;
    m_CurrentPipeline = &pipelineImpl;

//    if(m_CurrentPipeline->m_pipelineType == nri::PipelineMTL::Compute) {
 //       m_ComputeEncoder = [m_Handle computeCommandEncoderWithDescriptor: NULL];
  //  }

}
void CommandBufferMTL::SetPipelineLayout(const PipelineLayout& pipelineLayout) {

}
void CommandBufferMTL::SetDescriptorSet(uint32_t setIndexInPipelineLayout, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    
}
void CommandBufferMTL::SetConstants(uint32_t pushConstantIndex, const void* data, uint32_t size) {
    //if (pDesc->mUsedStages & SHADER_STAGE_VERT)
    //{
    //    [m_RendererEncoder setVertexBytes:data length:size atIndex:pushConstantIndex];
    //}

    //if (pDesc->mUsedStages & SHADER_STAGE_FRAG)
    //{
    //    [m_RendererEncoder  setFragmentBytes:data length:size atIndex:pushConstantIndex];
    //}

    //if (pDesc->mUsedStages & SHADER_STAGE_COMP)
    //{
    //    [m_RendererEncoder setBytes:data length:size atIndex:pushConstantIndex];
    //}

}
void CommandBufferMTL::SetDescriptorPool(const DescriptorPool& descriptorPool) {}
void CommandBufferMTL::Barrier(const BarrierGroupDesc& barrierGroupDesc) {
    //if (pCmd->pQueue->mBarrierFlags & BARRIER_FLAG_BUFFERS)
    {
        [m_RendererEncoder memoryBarrierWithScope:MTLBarrierScopeBuffers
                                         afterStages:MTLRenderStageFragment
                                        beforeStages:MTLRenderStageVertex];
    }

    //if (pCmd->pQueue->mBarrierFlags & BARRIER_FLAG_TEXTURES)
    {
        [m_RendererEncoder memoryBarrierWithScope:MTLBarrierScopeTextures
                                         afterStages:MTLRenderStageFragment
                                        beforeStages:MTLRenderStageVertex];
    }

    //if (pCmd->pQueue->mBarrierFlags & BARRIER_FLAG_RENDERTARGETS)
    {
        [m_RendererEncoder memoryBarrierWithScope:MTLBarrierScopeRenderTargets
                                         afterStages:MTLRenderStageFragment
                                        beforeStages:MTLRenderStageVertex];
    }

    
}
void CommandBufferMTL::BeginRendering(const AttachmentsDesc& attachmentsDesc) {
    MTLRenderPassDescriptor* renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    for(uint32_t i = 0; i < attachmentsDesc.colorNum; i++) {
        
    }
    
    //renderPassDescriptor.colorAttachments[
    
    //renderPassDescriptor.colorAttachments
    
    m_RendererEncoder = [m_Handle renderCommandEncoderWithDescriptor: NULL];
}
void CommandBufferMTL::EndRendering() {
    [m_RendererEncoder endEncoding];
    m_RendererEncoder = nil;
    m_ComputeEncoder = nil;
}
void CommandBufferMTL::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    MTLViewport* mtlViewports = StackAlloc(MTLViewport, viewportNum);

  //  [m_RendererEncoder setViewports:<#(const MTLViewport * _Nonnull)#> count:<#(NSUInteger)#>
}
void CommandBufferMTL::SetScissors(const Rect* rects, uint32_t rectNum) {
    NSCAssert(m_RendererEncoder, @"encoder set");
    MTLScissorRect rect;
    rect.x = rects[rectNum].x;
    rect.y = rects[rectNum].y;
    rect.width = rects[rectNum].width;
    rect.height = rects[rectNum].height;
    [m_RendererEncoder setScissorRect:rect];
}
void CommandBufferMTL::SetDepthBounds(float boundsMin, float boundsMax) {
}
void CommandBufferMTL::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    [m_RendererEncoder setStencilFrontReferenceValue: frontRef backReferenceValue:backRef];
}
void CommandBufferMTL::SetSamplePositions(const SamplePosition* positions, Sample_t positionNum, Sample_t sampleNum) {
    
}
void CommandBufferMTL::SetBlendConstants(const Color32f& color) {
    [m_RendererEncoder
     setBlendColorRed:color.x
     green:color.y 
     blue:color.z
     alpha:color.w
    ];
}
void CommandBufferMTL::SetShadingRate(const ShadingRateDesc& shadingRateDesc) {
    
}

void CommandBufferMTL::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
}

void CommandBufferMTL::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    m_CurrentIndexCmd.m_Buffer = &(BufferMTL&)buffer;
    switch(indexType) {
        case IndexType::UINT16:
            m_CurrentIndexCmd.m_Type = MTLIndexType::MTLIndexTypeUInt16;
            break;
        default:
        case IndexType::UINT32:
            m_CurrentIndexCmd.m_Type = MTLIndexType::MTLIndexTypeUInt32;
            break;
    }
    m_CurrentIndexCmd.m_Offset = offset;
}

void CommandBufferMTL::SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    for(size_t i = 0; i < bufferNum; i++) {
        BufferMTL* mtlBuffer = (BufferMTL*)buffers[i];
        [m_RendererEncoder setVertexBuffer: mtlBuffer->GetHandle()
                                offset: offsets[i]
                               atIndex: i + baseSlot];
    }
    
}

void CommandBufferMTL::Draw(const DrawDesc& drawDesc) {
    [m_RendererEncoder drawPrimitives: m_CurrentPipeline->m_primitiveType
            vertexStart:drawDesc.baseVertex
            vertexCount:drawDesc.vertexNum
            instanceCount:drawDesc.instanceNum
            baseInstance:0];
}

void CommandBufferMTL::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    id<MTLBuffer> indexBuffer = m_CurrentIndexCmd.m_Buffer->GetHandle();
    [m_RendererEncoder drawIndexedPrimitives: m_CurrentPipeline->m_primitiveType
                              indexCount:drawIndexedDesc.indexNum
                               indexType: m_CurrentIndexCmd.m_Type
                             indexBuffer: indexBuffer
                       indexBufferOffset: m_CurrentIndexCmd.m_Offset];
}

void CommandBufferMTL::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    // TODO: implement count Buffer
    NSCAssert(!countBuffer, @"count buffer not supported");
    [m_RendererEncoder
     drawPrimitives: m_CurrentPipeline->m_primitiveType
     indirectBuffer:((BufferMTL&)buffer).GetHandle()
     indirectBufferOffset: offset];
}
void CommandBufferMTL::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    
}
void CommandBufferMTL::Dispatch(const DispatchDesc& dispatchDesc) {
    
}
void CommandBufferMTL::DispatchIndirect(const Buffer& buffer, uint64_t offset) {}
void CommandBufferMTL::BeginQuery(const QueryPool& queryPool, uint32_t offset) {}
void CommandBufferMTL::EndQuery(const QueryPool& queryPool, uint32_t offset) {}
void CommandBufferMTL::BeginAnnotation(const char* name) {}
void CommandBufferMTL::EndAnnotation() {}
void CommandBufferMTL::ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc) {}
void CommandBufferMTL::ClearStorageTexture(const ClearStorageTextureDesc& clearDesc) {}
void CommandBufferMTL::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {}
void CommandBufferMTL::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {}
void CommandBufferMTL::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {}
void CommandBufferMTL::ReadbackTextureToBuffer(Buffer& dstBuffer, TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {}
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

void Create(id<MTLCommandBuffer> cmd);



#include "CommandBufferMTL.hpp"

