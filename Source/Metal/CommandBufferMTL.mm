#include "SharedMTL.h"

#include "CommandBufferMTL.h"

//#include "AccelerationStructureVK.h"
#include "BufferMTL.h"
#include "CommandBufferMTL.h"
#include "CommandQueueMTL.h"
//#include "DescriptorSetMTL.h"
#include "DescriptorMTL.h"
#include "PipelineLayoutMTL.h"
#include "PipelineMTL.h"
//#include "QueryPoolMTL.h"
#include "TextureMTL.h"

#include <math.h>

using namespace nri;

CommandBufferMTL::~CommandBufferMTL() {

}

void CommandBufferMTL::SetDebugName(const char* name) {
    [m_Handle setLabel:[NSString stringWithUTF8String:name]];
}

Result CommandBufferMTL::Begin(const DescriptorPool* descriptorPool) {
    
}
Result CommandBufferMTL::End() {
    
}
void CommandBufferMTL::SetPipeline(const Pipeline& pipeline) {
    if (m_CurrentPipeline == (PipelineMTL*)&pipeline)
        return;
    PipelineMTL& pipelineImpl = (PipelineMTL&)pipeline;
    m_CurrentPipeline = &pipelineImpl;
    

}
void CommandBufferMTL::SetPipelineLayout(const PipelineLayout& pipelineLayout) {}
void CommandBufferMTL::SetDescriptorSet(uint32_t setIndexInPipelineLayout, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {}
void CommandBufferMTL::SetConstants(uint32_t pushConstantIndex, const void* data, uint32_t size) {}
void CommandBufferMTL::SetDescriptorPool(const DescriptorPool& descriptorPool) {}
void CommandBufferMTL::Barrier(const BarrierGroupDesc& barrierGroupDesc) {}
void CommandBufferMTL::BeginRendering(const AttachmentsDesc& attachmentsDesc) {
    m_encoder = [m_Handle renderCommandEncoderWithDescriptor: NULL];
    
}
void CommandBufferMTL::EndRendering() {}
void CommandBufferMTL::SetViewports(const Viewport* viewports, uint32_t viewportNum) {}
void CommandBufferMTL::SetScissors(const Rect* rects, uint32_t rectNum) {}
void CommandBufferMTL::SetDepthBounds(float boundsMin, float boundsMax) {}
void CommandBufferMTL::SetStencilReference(uint8_t frontRef, uint8_t backRef) {}
void CommandBufferMTL::SetSamplePositions(const SamplePosition* positions, Sample_t positionNum, Sample_t sampleNum) {}
void CommandBufferMTL::SetBlendConstants(const Color32f& color) {}
void CommandBufferMTL::SetShadingRate(const ShadingRateDesc& shadingRateDesc) {}
void CommandBufferMTL::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {}
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
}
void CommandBufferMTL::Draw(const DrawDesc& drawDesc) {
    
    [m_encoder drawPrimitives: m_CurrentPipeline->m_graphics.m_primitiveType
            vertexStart:drawDesc.baseVertex
            vertexCount:drawDesc.vertexNum
            instanceCount:drawDesc.instanceNum
               baseInstance:0];
}
void CommandBufferMTL::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    id<MTLBuffer> indexBuffer = m_CurrentIndexCmd.m_Buffer->GetHandle();
    [m_encoder drawIndexedPrimitives: m_CurrentPipeline->m_graphics.m_primitiveType
                              indexCount:drawIndexedDesc.indexNum
                               indexType: m_CurrentIndexCmd.m_Type
                             indexBuffer: indexBuffer
                       indexBufferOffset: m_CurrentIndexCmd.m_Offset];
}
void CommandBufferMTL::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {}
void CommandBufferMTL::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {}
void CommandBufferMTL::Dispatch(const DispatchDesc& dispatchDesc) {}
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
void CommandBufferMTL::DrawMeshTasksIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride) {}

#include "CommandBufferMTL.hpp"

