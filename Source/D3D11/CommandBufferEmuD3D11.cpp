// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"

#include "CommandBufferD3D11.h"
#include "CommandBufferEmuD3D11.h"
#include "DescriptorSetD3D11.h"
#include "PipelineD3D11.h"
#include "PipelineLayoutD3D11.h"

using namespace nri;

enum OpCode : uint32_t {
    BEGIN,
    END,
    SET_VIEWPORTS,
    SET_SCISSORS,
    SET_DEPTH_BOUNDS,
    SET_STENCIL_REFERENCE,
    SET_SAMPLE_POSITIONS,
    SET_BLEND_CONSTANTS,
    CLEAR_ATTACHMENTS,
    CLEAR_STORAGE_BUFFER,
    CLEAR_STORAGE_TEXTURE,
    BEGIN_RENDERING,
    END_RENDERING,
    BIND_VERTEX_BUFFERS,
    BIND_INDEX_BUFFER,
    BIND_PIPELINE_LAYOUT,
    BIND_PIPELINE,
    BIND_DESCRIPTOR_SET,
    SET_CONSTANTS,
    DRAW,
    DRAW_INDEXED,
    DRAW_INDIRECT,
    DRAW_INDEXED_INDIRECT,
    COPY_BUFFER,
    COPY_TEXTURE,
    UPLOAD_BUFFER_TO_TEXTURE,
    READBACK_TEXTURE_TO_BUFFER,
    DISPATCH,
    DISPATCH_INDIRECT,
    BARRIER,
    BEGIN_QUERY,
    END_QUERY,
    COPY_QUERIES,
    BEGIN_ANNOTATION,
    END_ANNOTATION,

    UNKNOWN
};

inline size_t GetElementNum(size_t dataSize) {
    return (dataSize + sizeof(uint32_t) - 1) / sizeof(uint32_t);
}

template <typename T>
inline void Push(PushBuffer& pushBuffer, const T& data) {
    const size_t bytes = sizeof(T);
    const size_t newElements = GetElementNum(bytes);
    const size_t curr = pushBuffer.size();

    pushBuffer.resize(curr + newElements);

    uint32_t* p = &pushBuffer[curr];
    memcpy(p, &data, bytes);
}

template <typename T>
inline void Push(PushBuffer& pushBuffer, const T* data, uint32_t num) {
    const size_t bytes = sizeof(T) * num;
    const size_t newElements = GetElementNum(sizeof(uint32_t) + bytes);
    const size_t curr = pushBuffer.size();

    pushBuffer.resize(curr + newElements);

    uint32_t* p = &pushBuffer[curr];
    *p++ = num;
    memcpy(p, data, bytes);
}

template <typename T>
inline void Read(PushBuffer& pushBuffer, size_t& i, T& data) {
    data = *(T*)&pushBuffer[i];
    i += GetElementNum(sizeof(T));
}

template <typename T>
inline void Read(PushBuffer& pushBuffer, size_t& i, T*& data, uint32_t& num) {
    num = pushBuffer[i++];
    data = (T*)&pushBuffer[i];
    i += GetElementNum(sizeof(T) * num);
}

template <typename T>
inline void Read(PushBuffer& pushBuffer, size_t& i, T*& data, uint16_t& num) {
    num = (uint16_t)pushBuffer[i++];
    data = (T*)&pushBuffer[i];
    i += GetElementNum(sizeof(T) * num);
}

//================================================================================================================
// CommandBufferHelper
//================================================================================================================

Result CommandBufferEmuD3D11::Create(ID3D11DeviceContext* precreatedContext) {
    MaybeUnused(precreatedContext);

    m_PushBuffer.reserve(256);

    return Result::SUCCESS;
}

void CommandBufferEmuD3D11::Submit() {
    CommandBufferD3D11 commandBuffer(m_Device);

    OpCode opCode = UNKNOWN;
    size_t i = 0;

    while (i < m_PushBuffer.size()) {
        Read(m_PushBuffer, i, opCode);

        switch (opCode) {
            case BEGIN: {
                DescriptorPool* descriptorPool;
                Read(m_PushBuffer, i, descriptorPool);

                if (descriptorPool)
                    commandBuffer.SetDescriptorPool(*descriptorPool);
            } break;
            case END:
                // we should return default state in emulation mode!
                commandBuffer.SetDepthBounds(0.0f, 1.0f);
                break;
            case SET_VIEWPORTS: {
                uint32_t viewportNum;
                Viewport* viewports;
                Read(m_PushBuffer, i, viewports, viewportNum);

                commandBuffer.SetViewports(viewports, viewportNum);
            } break;
            case SET_SCISSORS: {
                uint32_t rectNum;
                Rect* rects;
                Read(m_PushBuffer, i, rects, rectNum);

                commandBuffer.SetScissors(rects, rectNum);
            } break;
            case SET_DEPTH_BOUNDS: {
                float boundsMin;
                float boundsMax;
                Read(m_PushBuffer, i, boundsMin);
                Read(m_PushBuffer, i, boundsMax);

                commandBuffer.SetDepthBounds(boundsMin, boundsMax);
            } break;
            case SET_STENCIL_REFERENCE: {
                uint8_t frontRef;
                uint8_t backRef;
                Read(m_PushBuffer, i, frontRef);
                Read(m_PushBuffer, i, backRef);

                commandBuffer.SetStencilReference(frontRef, backRef);
            } break;
            case SET_SAMPLE_POSITIONS: {
                SamplePosition* positions;
                uint32_t positionNum;
                Sample_t sampleNum;
                Read(m_PushBuffer, i, positions, positionNum);
                Read(m_PushBuffer, i, sampleNum);

                commandBuffer.SetSamplePositions(positions, (Sample_t)positionNum, sampleNum);
            } break;
            case SET_BLEND_CONSTANTS: {
                Color32f color;
                Read(m_PushBuffer, i, color);

                commandBuffer.SetBlendConstants(color);
            } break;
            case CLEAR_ATTACHMENTS: {
                ClearDesc* clearDescs;
                uint32_t clearDescNum;
                Read(m_PushBuffer, i, clearDescs, clearDescNum);

                Rect* rects;
                uint32_t rectNum;
                Read(m_PushBuffer, i, rects, rectNum);

                commandBuffer.ClearAttachments(clearDescs, clearDescNum, rects, rectNum);
            } break;
            case CLEAR_STORAGE_BUFFER: {
                ClearStorageBufferDesc clearDesc = {};
                Read(m_PushBuffer, i, clearDesc);

                commandBuffer.ClearStorageBuffer(clearDesc);
            } break;
            case CLEAR_STORAGE_TEXTURE: {
                ClearStorageTextureDesc clearDesc = {};
                Read(m_PushBuffer, i, clearDesc);

                commandBuffer.ClearStorageTexture(clearDesc);
            } break;
            case BEGIN_RENDERING: {
                AttachmentsDesc attachmentsDesc = {};
                Read(m_PushBuffer, i, attachmentsDesc.colors, attachmentsDesc.colorNum);
                Read(m_PushBuffer, i, attachmentsDesc.depthStencil);

                commandBuffer.BeginRendering(attachmentsDesc);
            } break;
            case END_RENDERING: {
                commandBuffer.EndRendering();
            } break;
            case BIND_VERTEX_BUFFERS: {
                uint32_t baseSlot;
                Read(m_PushBuffer, i, baseSlot);

                Buffer** buffers;
                uint32_t bufferNum;
                Read(m_PushBuffer, i, buffers, bufferNum);

                uint64_t* offsets;
                uint32_t offsetNum;
                Read(m_PushBuffer, i, offsets, offsetNum);

                commandBuffer.SetVertexBuffers(baseSlot, bufferNum, buffers, offsets);
            } break;
            case BIND_INDEX_BUFFER: {
                Buffer* buffer;
                Read(m_PushBuffer, i, buffer);

                uint64_t offset;
                Read(m_PushBuffer, i, offset);

                IndexType indexType;
                Read(m_PushBuffer, i, indexType);

                commandBuffer.SetIndexBuffer(*buffer, offset, indexType);
            } break;
            case BIND_PIPELINE_LAYOUT: {
                PipelineLayout* pipelineLayout;
                Read(m_PushBuffer, i, pipelineLayout);

                commandBuffer.SetPipelineLayout(*pipelineLayout);
            } break;
            case BIND_PIPELINE: {
                Pipeline* pipeline;
                Read(m_PushBuffer, i, pipeline);

                commandBuffer.SetPipeline(*pipeline);
            } break;
            case BIND_DESCRIPTOR_SET: {
                uint32_t setIndexInPipelineLayout;
                Read(m_PushBuffer, i, setIndexInPipelineLayout);

                DescriptorSet* descriptorSet;
                Read(m_PushBuffer, i, descriptorSet);

                uint32_t* dynamicConstantBufferOffsets;
                uint32_t dynamicConstantBufferNum;
                Read(m_PushBuffer, i, dynamicConstantBufferOffsets, dynamicConstantBufferNum);

                commandBuffer.SetDescriptorSet(setIndexInPipelineLayout, *descriptorSet, dynamicConstantBufferOffsets);
            } break;
            case SET_CONSTANTS: {
                uint32_t pushConstantRangeIndex;
                Read(m_PushBuffer, i, pushConstantRangeIndex);

                uint8_t* data;
                uint32_t size;
                Read(m_PushBuffer, i, data, size);

                commandBuffer.SetConstants(pushConstantRangeIndex, data, size);
            } break;
            case DRAW: {
                DrawDesc drawDesc = {};
                Read(m_PushBuffer, i, drawDesc);

                commandBuffer.Draw(drawDesc);
            } break;
            case DRAW_INDEXED: {
                DrawIndexedDesc drawIndexedDesc = {};
                Read(m_PushBuffer, i, drawIndexedDesc);

                commandBuffer.DrawIndexed(drawIndexedDesc);
            } break;
            case DRAW_INDIRECT: {
                Buffer* buffer;
                Read(m_PushBuffer, i, buffer);

                uint64_t offset;
                Read(m_PushBuffer, i, offset);

                uint32_t drawNum;
                Read(m_PushBuffer, i, drawNum);

                uint32_t stride;
                Read(m_PushBuffer, i, stride);

                Buffer* countBuffer;
                Read(m_PushBuffer, i, countBuffer);

                uint64_t countBufferOffset;
                Read(m_PushBuffer, i, countBufferOffset);

                commandBuffer.DrawIndirect(*buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
            } break;
            case DRAW_INDEXED_INDIRECT: {
                Buffer* buffer;
                Read(m_PushBuffer, i, buffer);

                uint64_t offset;
                Read(m_PushBuffer, i, offset);

                uint32_t drawNum;
                Read(m_PushBuffer, i, drawNum);

                uint32_t stride;
                Read(m_PushBuffer, i, stride);

                Buffer* countBuffer;
                Read(m_PushBuffer, i, countBuffer);

                uint64_t countBufferOffset;
                Read(m_PushBuffer, i, countBufferOffset);

                commandBuffer.DrawIndexedIndirect(*buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
            } break;
            case COPY_BUFFER: {
                Buffer* dstBuffer;
                Read(m_PushBuffer, i, dstBuffer);

                uint64_t dstOffset;
                Read(m_PushBuffer, i, dstOffset);

                Buffer* srcBuffer;
                Read(m_PushBuffer, i, srcBuffer);

                uint64_t srcOffset;
                Read(m_PushBuffer, i, srcOffset);

                uint64_t size;
                Read(m_PushBuffer, i, size);

                commandBuffer.CopyBuffer(*dstBuffer, dstOffset, *srcBuffer, srcOffset, size);
            } break;
            case COPY_TEXTURE: {
                Texture* dstTexture;
                Read(m_PushBuffer, i, dstTexture);

                TextureRegionDesc dstRegion = {};
                Read(m_PushBuffer, i, dstRegion);

                Texture* srcTexture;
                Read(m_PushBuffer, i, srcTexture);

                TextureRegionDesc srcRegion = {};
                Read(m_PushBuffer, i, srcRegion);

                commandBuffer.CopyTexture(*dstTexture, &dstRegion, *srcTexture, &srcRegion);
            } break;
            case UPLOAD_BUFFER_TO_TEXTURE: {
                Texture* dstTexture;
                Read(m_PushBuffer, i, dstTexture);

                TextureRegionDesc dstRegion = {};
                Read(m_PushBuffer, i, dstRegion);

                Buffer* srcBuffer;
                Read(m_PushBuffer, i, srcBuffer);

                TextureDataLayoutDesc srcDataLayout = {};
                Read(m_PushBuffer, i, srcDataLayout);

                commandBuffer.UploadBufferToTexture(*dstTexture, dstRegion, *srcBuffer, srcDataLayout);
            } break;
            case READBACK_TEXTURE_TO_BUFFER: {
                Buffer* dstBuffer;
                Read(m_PushBuffer, i, dstBuffer);

                TextureDataLayoutDesc dstDataLayout = {};
                Read(m_PushBuffer, i, dstDataLayout);

                Texture* srcTexture;
                Read(m_PushBuffer, i, srcTexture);

                TextureRegionDesc srcRegion = {};
                Read(m_PushBuffer, i, srcRegion);

                commandBuffer.ReadbackTextureToBuffer(*dstBuffer, dstDataLayout, *srcTexture, srcRegion);
            } break;
            case DISPATCH: {
                DispatchDesc dispatchDesc;
                Read(m_PushBuffer, i, dispatchDesc);

                commandBuffer.Dispatch(dispatchDesc);
            } break;
            case DISPATCH_INDIRECT: {
                Buffer* buffer;
                Read(m_PushBuffer, i, buffer);

                uint64_t offset;
                Read(m_PushBuffer, i, offset);

                commandBuffer.DispatchIndirect(*buffer, offset);
            } break;
            case BARRIER: {
                BarrierGroupDesc barrierGroupDesc = {};
                Read(m_PushBuffer, i, barrierGroupDesc.globals, barrierGroupDesc.globalNum);
                Read(m_PushBuffer, i, barrierGroupDesc.buffers, barrierGroupDesc.bufferNum);
                Read(m_PushBuffer, i, barrierGroupDesc.textures, barrierGroupDesc.textureNum);

                commandBuffer.Barrier(barrierGroupDesc);
            } break;
            case BEGIN_QUERY: {
                QueryPool* queryPool;
                Read(m_PushBuffer, i, queryPool);

                uint32_t offset;
                Read(m_PushBuffer, i, offset);

                commandBuffer.BeginQuery(*queryPool, offset);
            } break;
            case END_QUERY: {
                QueryPool* queryPool;
                Read(m_PushBuffer, i, queryPool);

                uint32_t offset;
                Read(m_PushBuffer, i, offset);

                commandBuffer.EndQuery(*queryPool, offset);
            } break;
            case COPY_QUERIES: {
                QueryPool* queryPool;
                Read(m_PushBuffer, i, queryPool);

                uint32_t offset;
                Read(m_PushBuffer, i, offset);

                uint32_t num;
                Read(m_PushBuffer, i, num);

                Buffer* buffer;
                Read(m_PushBuffer, i, buffer);

                uint64_t alignedBufferOffset;
                Read(m_PushBuffer, i, alignedBufferOffset);

                commandBuffer.CopyQueries(*queryPool, offset, num, *buffer, alignedBufferOffset);
            } break;
            case BEGIN_ANNOTATION: {
                uint32_t len;
                const char* name;
                Read(m_PushBuffer, i, name, len);

                commandBuffer.BeginAnnotation(name);
            } break;
            case END_ANNOTATION: {
                commandBuffer.EndAnnotation();
            } break;
        }
    }
}

ID3D11DeviceContext* CommandBufferEmuD3D11::GetNativeObject() const {
    return m_Device.GetImmediateContext();
}

StdAllocator<uint8_t>& CommandBufferEmuD3D11::GetStdAllocator() const {
    return m_Device.GetStdAllocator();
}

//================================================================================================================
// NRI
//================================================================================================================

inline Result CommandBufferEmuD3D11::Begin(const DescriptorPool* descriptorPool) {
    m_PushBuffer.clear();
    Push(m_PushBuffer, BEGIN);
    Push(m_PushBuffer, descriptorPool);

    return Result::SUCCESS;
}

inline Result CommandBufferEmuD3D11::End() {
    Push(m_PushBuffer, END);

    return Result::SUCCESS;
}

inline void CommandBufferEmuD3D11::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    Push(m_PushBuffer, SET_VIEWPORTS);
    Push(m_PushBuffer, viewports, viewportNum);
}

inline void CommandBufferEmuD3D11::SetScissors(const Rect* rects, uint32_t rectNum) {
    Push(m_PushBuffer, SET_SCISSORS);
    Push(m_PushBuffer, rects, rectNum);
}

inline void CommandBufferEmuD3D11::SetDepthBounds(float boundsMin, float boundsMax) {
    Push(m_PushBuffer, SET_DEPTH_BOUNDS);
    Push(m_PushBuffer, boundsMin);
    Push(m_PushBuffer, boundsMax);
}

inline void CommandBufferEmuD3D11::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    Push(m_PushBuffer, SET_STENCIL_REFERENCE);
    Push(m_PushBuffer, frontRef);
    Push(m_PushBuffer, backRef);
}

inline void CommandBufferEmuD3D11::SetSamplePositions(const SamplePosition* positions, Sample_t positionNum, Sample_t sampleNum) {
    Push(m_PushBuffer, SET_SAMPLE_POSITIONS);
    Push(m_PushBuffer, positions, (uint32_t)positionNum);
    Push(m_PushBuffer, sampleNum);
}

inline void CommandBufferEmuD3D11::SetBlendConstants(const Color32f& color) {
    Push(m_PushBuffer, SET_BLEND_CONSTANTS);
    Push(m_PushBuffer, color);
}

inline void CommandBufferEmuD3D11::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    Push(m_PushBuffer, CLEAR_ATTACHMENTS);
    Push(m_PushBuffer, clearDescs, clearDescNum);
    Push(m_PushBuffer, rects, rectNum);
}

inline void CommandBufferEmuD3D11::ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc) {
    Push(m_PushBuffer, CLEAR_STORAGE_BUFFER);
    Push(m_PushBuffer, clearDesc);
}

inline void CommandBufferEmuD3D11::ClearStorageTexture(const ClearStorageTextureDesc& clearDesc) {
    Push(m_PushBuffer, CLEAR_STORAGE_TEXTURE);
    Push(m_PushBuffer, clearDesc);
}

inline void CommandBufferEmuD3D11::BeginRendering(const AttachmentsDesc& attachmentsDesc) {
    Push(m_PushBuffer, BEGIN_RENDERING);
    Push(m_PushBuffer, attachmentsDesc.colors, attachmentsDesc.colorNum);
    Push(m_PushBuffer, attachmentsDesc.depthStencil);
}

inline void CommandBufferEmuD3D11::EndRendering() {
    Push(m_PushBuffer, END_RENDERING);
}

inline void CommandBufferEmuD3D11::SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    Push(m_PushBuffer, BIND_VERTEX_BUFFERS);
    Push(m_PushBuffer, baseSlot);
    Push(m_PushBuffer, buffers, bufferNum);
    Push(m_PushBuffer, offsets, bufferNum);
}

inline void CommandBufferEmuD3D11::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    Push(m_PushBuffer, BIND_INDEX_BUFFER);
    Push(m_PushBuffer, &buffer);
    Push(m_PushBuffer, offset);
    Push(m_PushBuffer, indexType);
}

inline void CommandBufferEmuD3D11::SetPipelineLayout(const PipelineLayout& pipelineLayout) {
    Push(m_PushBuffer, BIND_PIPELINE_LAYOUT);
    Push(m_PushBuffer, &pipelineLayout);
}

inline void CommandBufferEmuD3D11::SetPipeline(const Pipeline& pipeline) {
    Push(m_PushBuffer, BIND_PIPELINE);
    Push(m_PushBuffer, &pipeline);
}

inline void CommandBufferEmuD3D11::SetDescriptorPool(const DescriptorPool& descriptorPool) {
    MaybeUnused(descriptorPool);
}

inline void CommandBufferEmuD3D11::SetDescriptorSet(uint32_t setIndexInPipelineLayout, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    uint32_t dynamicConstantBufferNum = ((DescriptorSetD3D11&)descriptorSet).GetDynamicConstantBufferNum();

    Push(m_PushBuffer, BIND_DESCRIPTOR_SET);
    Push(m_PushBuffer, setIndexInPipelineLayout);
    Push(m_PushBuffer, &descriptorSet);
    Push(m_PushBuffer, dynamicConstantBufferOffsets, dynamicConstantBufferNum);
}

inline void CommandBufferEmuD3D11::SetConstants(uint32_t pushConstantIndex, const void* data, uint32_t size) {
    Push(m_PushBuffer, SET_CONSTANTS);
    Push(m_PushBuffer, pushConstantIndex);
    Push(m_PushBuffer, (uint8_t*)data, size);
}

inline void CommandBufferEmuD3D11::Draw(const DrawDesc& drawDesc) {
    Push(m_PushBuffer, DRAW);
    Push(m_PushBuffer, drawDesc);
}

inline void CommandBufferEmuD3D11::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    Push(m_PushBuffer, DRAW_INDEXED);
    Push(m_PushBuffer, drawIndexedDesc);
}

inline void CommandBufferEmuD3D11::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    Push(m_PushBuffer, DRAW_INDIRECT);
    Push(m_PushBuffer, &buffer);
    Push(m_PushBuffer, offset);
    Push(m_PushBuffer, drawNum);
    Push(m_PushBuffer, stride);
    Push(m_PushBuffer, countBuffer);
    Push(m_PushBuffer, countBufferOffset);
}

inline void CommandBufferEmuD3D11::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    Push(m_PushBuffer, DRAW_INDEXED_INDIRECT);
    Push(m_PushBuffer, &buffer);
    Push(m_PushBuffer, offset);
    Push(m_PushBuffer, drawNum);
    Push(m_PushBuffer, stride);
    Push(m_PushBuffer, countBuffer);
    Push(m_PushBuffer, countBufferOffset);
}

inline void CommandBufferEmuD3D11::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    Push(m_PushBuffer, COPY_BUFFER);
    Push(m_PushBuffer, &dstBuffer);
    Push(m_PushBuffer, dstOffset);
    Push(m_PushBuffer, &srcBuffer);
    Push(m_PushBuffer, srcOffset);
    Push(m_PushBuffer, size);
}

inline void CommandBufferEmuD3D11::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    TextureRegionDesc fullResource = {};
    fullResource.mipOffset = NULL_TEXTURE_REGION_DESC;

    if (!dstRegionDesc)
        dstRegionDesc = &fullResource;
    if (!srcRegionDesc)
        srcRegionDesc = &fullResource;

    Push(m_PushBuffer, COPY_TEXTURE);
    Push(m_PushBuffer, &dstTexture);
    Push(m_PushBuffer, *dstRegionDesc);
    Push(m_PushBuffer, &srcTexture);
    Push(m_PushBuffer, *srcRegionDesc);
}

inline void CommandBufferEmuD3D11::UploadBufferToTexture(
    Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {
    Push(m_PushBuffer, UPLOAD_BUFFER_TO_TEXTURE);
    Push(m_PushBuffer, &dstTexture);
    Push(m_PushBuffer, dstRegionDesc);
    Push(m_PushBuffer, &srcBuffer);
    Push(m_PushBuffer, srcDataLayoutDesc);
}

inline void CommandBufferEmuD3D11::ReadbackTextureToBuffer(
    Buffer& dstBuffer, TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {
    Push(m_PushBuffer, READBACK_TEXTURE_TO_BUFFER);
    Push(m_PushBuffer, &dstBuffer);
    Push(m_PushBuffer, dstDataLayoutDesc);
    Push(m_PushBuffer, &srcTexture);
    Push(m_PushBuffer, srcRegionDesc);
}

inline void CommandBufferEmuD3D11::Dispatch(const DispatchDesc& dispatchDesc) {
    Push(m_PushBuffer, DISPATCH);
    Push(m_PushBuffer, dispatchDesc);
}

inline void CommandBufferEmuD3D11::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    Push(m_PushBuffer, DISPATCH_INDIRECT);
    Push(m_PushBuffer, &buffer);
    Push(m_PushBuffer, offset);
}

inline void CommandBufferEmuD3D11::Barrier(const BarrierGroupDesc& barrierGroupDesc) {
    Push(m_PushBuffer, BARRIER);
    Push(m_PushBuffer, barrierGroupDesc.globals, barrierGroupDesc.globalNum);
    Push(m_PushBuffer, barrierGroupDesc.buffers, barrierGroupDesc.bufferNum);
    Push(m_PushBuffer, barrierGroupDesc.textures, barrierGroupDesc.textureNum);
}

inline void CommandBufferEmuD3D11::BeginQuery(const QueryPool& queryPool, uint32_t offset) {
    Push(m_PushBuffer, BEGIN_QUERY);
    Push(m_PushBuffer, &queryPool);
    Push(m_PushBuffer, offset);
}

inline void CommandBufferEmuD3D11::EndQuery(const QueryPool& queryPool, uint32_t offset) {
    Push(m_PushBuffer, END_QUERY);
    Push(m_PushBuffer, &queryPool);
    Push(m_PushBuffer, offset);
}

inline void CommandBufferEmuD3D11::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    Push(m_PushBuffer, COPY_QUERIES);
    Push(m_PushBuffer, &queryPool);
    Push(m_PushBuffer, offset);
    Push(m_PushBuffer, num);
    Push(m_PushBuffer, &dstBuffer);
    Push(m_PushBuffer, dstOffset);
}

inline void CommandBufferEmuD3D11::BeginAnnotation(const char* name) {
    uint32_t len = (uint32_t)std::strlen(name) + 1;

    Push(m_PushBuffer, BEGIN_ANNOTATION);
    Push(m_PushBuffer, name, len);
}

inline void CommandBufferEmuD3D11::EndAnnotation() {
    Push(m_PushBuffer, END_ANNOTATION);
}

#include "CommandBufferEmuD3D11.hpp"
