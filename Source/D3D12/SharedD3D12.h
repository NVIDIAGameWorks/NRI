// © 2021 NVIDIA Corporation

#pragma once

#include <d3d12.h>

#include "SharedExternal.h"

typedef size_t DescriptorPointerCPU;
typedef uint64_t DescriptorPointerGPU;
typedef uint16_t HeapIndexType;
typedef uint16_t HeapOffsetType;

#define BASE_ATTRIBUTES_EMULATION_SPACE 999 // see NRI_ENABLE_DRAW_PARAMETERS

namespace nri {
enum DescriptorHeapType : uint32_t {
    RESOURCE = 0,
    SAMPLER,
    MAX_NUM
};

struct DescriptorHandle {
    HeapIndexType heapIndex;
    HeapOffsetType heapOffset;
};

struct DescriptorHeapDesc {
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    DescriptorPointerCPU descriptorPointerCPU;
    DescriptorPointerGPU descriptorPointerGPU;
    uint32_t descriptorSize;
};

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE GetAccelerationStructureType(AccelerationStructureType accelerationStructureType);
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS GetAccelerationStructureBuildFlags(AccelerationStructureBuildBits accelerationStructureBuildFlags);
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE GetCopyMode(CopyMode copyMode);

D3D12_RESOURCE_FLAGS GetBufferFlags(BufferUsageBits bufferUsageMask);
D3D12_RESOURCE_FLAGS GetTextureFlags(TextureUsageBits textureUsageMask);

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
D3D12_SHADER_VISIBILITY GetShaderVisibility(StageBits shaderStage);
D3D12_DESCRIPTOR_RANGE_TYPE GetDescriptorRangesType(DescriptorType descriptorType);
D3D12_RESOURCE_DIMENSION GetResourceDimension(TextureType textureType);
} // namespace nri

#if NRI_USE_EXT_LIBS
#    include "amdags/ags_lib/inc/amd_ags.h"
#    include "nvapi/nvShaderExtnEnums.h"
#    include "nvapi/nvapi.h"
#endif

namespace d3d12 {
#include "D3DExt.h"
}

#include "DeviceD3D12.h"
