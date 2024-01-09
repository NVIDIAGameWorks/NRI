// © 2021 NVIDIA Corporation

#pragma once

#include <d3d12.h>

#include "SharedExternal.h"
#include "DeviceBase.h"
#include "DeviceD3D12.h"

#define NRI_TEMP_NODE_MASK 0x1

namespace nri
{
enum DescriptorHeapType : uint32_t
{
    RESOURCE = 0,
    SAMPLER,
    MAX_NUM
};

DXGI_FORMAT GetShaderFormatForDepth(DXGI_FORMAT format);

MemoryType GetMemoryType(D3D12_HEAP_TYPE heapType, D3D12_HEAP_FLAGS heapFlags);
MemoryType GetMemoryType(MemoryLocation memoryLocation, const D3D12_RESOURCE_DESC& resourceDesc);

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE GetAccelerationStructureType(AccelerationStructureType accelerationStructureType);
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS GetAccelerationStructureBuildFlags(AccelerationStructureBuildBits accelerationStructureBuildFlags);
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE GetCopyMode(CopyMode copyMode);

D3D12_RESOURCE_FLAGS GetBufferFlags(BufferUsageBits bufferUsageMask);
D3D12_RESOURCE_FLAGS GetTextureFlags(TextureUsageBits textureUsageMask);
D3D12_RESOURCE_STATES GetResourceStates(AccessBits accessMask, D3D12_COMMAND_LIST_TYPE commandListType);

bool RequiresDedicatedAllocation(MemoryType memoryType);
void ConvertGeometryDescs(D3D12_RAYTRACING_GEOMETRY_DESC* geometryDescs, const GeometryObject* geometryObjects, uint32_t geometryObjectNum);
void ConvertRects(D3D12_RECT* rectsD3D12, const Rect* rects, uint32_t rectNum);
bool GetTextureDesc(const TextureD3D12Desc& textureD3D12Desc, TextureDesc& textureDesc);
bool GetBufferDesc(const BufferD3D12Desc& bufferD3D12Desc, BufferDesc& bufferDesc);
uint64_t GetMemorySizeD3D12(const MemoryD3D12Desc& memoryD3D12Desc);

D3D12_FILTER GetFilterIsotropic(Filter mip, Filter magnification, Filter minification, FilterExt filterExt, bool useComparison);
D3D12_FILTER GetFilterAnisotropic(FilterExt filterExt, bool useComparison);
D3D12_TEXTURE_ADDRESS_MODE GetAddressMode(AddressMode addressMode);
D3D12_COMPARISON_FUNC GetComparisonFunc(CompareFunc compareFunc);
D3D12_COMMAND_LIST_TYPE GetCommandListType(CommandQueueType commandQueueType);
D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorHeapType(DescriptorType descriptorType);
D3D12_HEAP_TYPE GetHeapType(MemoryType memoryType);
D3D12_HEAP_FLAGS GetHeapFlags(MemoryType memoryType);
D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType(Topology topology);
D3D_PRIMITIVE_TOPOLOGY GetPrimitiveTopology(Topology topology, uint8_t tessControlPointNum);
D3D12_FILL_MODE GetFillMode(FillMode fillMode);
D3D12_CULL_MODE GetCullMode(CullMode cullMode);
D3D12_STENCIL_OP GetStencilOp(StencilFunc stencilFunc);
UINT8 GetRenderTargetWriteMask(ColorWriteBits colorWriteMask);
D3D12_LOGIC_OP GetLogicOp(LogicFunc logicFunc);
D3D12_BLEND GetBlend(BlendFactor blendFactor);
D3D12_BLEND_OP GetBlendOp(BlendFunc blendFunc);
D3D12_SHADER_VISIBILITY GetShaderVisibility(ShaderStage shaderStage);
D3D12_DESCRIPTOR_RANGE_TYPE GetDescriptorRangesType(DescriptorType descriptorType);
D3D12_RESOURCE_DIMENSION GetResourceDimension(TextureType textureType);

D3D12_QUERY_TYPE GetQueryType(QueryType queryType);
D3D12_QUERY_HEAP_TYPE GetQueryHeapType(QueryType queryType);
uint32_t GetQueryElementSize(D3D12_QUERY_TYPE queryType);
}