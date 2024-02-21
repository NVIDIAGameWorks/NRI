// © 2021 NVIDIA Corporation

#include "SharedD3D12.h"

#include "BufferD3D12.h"

using namespace nri;

constexpr std::array<D3D12_COMMAND_LIST_TYPE, (uint32_t)CommandQueueType::MAX_NUM> COMMAND_LIST_TYPES = {
    D3D12_COMMAND_LIST_TYPE_DIRECT,  // GRAPHICS
    D3D12_COMMAND_LIST_TYPE_COMPUTE, // COMPUTE
    D3D12_COMMAND_LIST_TYPE_COPY     // COPY
};

D3D12_COMMAND_LIST_TYPE nri::GetCommandListType(CommandQueueType commandQueueType) {
    return COMMAND_LIST_TYPES[(uint32_t)commandQueueType];
}

constexpr std::array<D3D12_HEAP_TYPE, (uint32_t)MemoryLocation::MAX_NUM> HEAP_TYPES = {
    D3D12_HEAP_TYPE_DEFAULT, // DEVICE
#ifdef NRI_USE_AGILITY_SDK
    // Prerequisite: D3D12_FEATURE_D3D12_OPTIONS16
    D3D12_HEAP_TYPE_GPU_UPLOAD, // DEVICE_UPLOAD
#else
    D3D12_HEAP_TYPE_UPLOAD, // DEVICE_UPLOAD (silent fallback to HOST_UPLOAD)
#endif
    D3D12_HEAP_TYPE_UPLOAD,   // HOST_UPLOAD
    D3D12_HEAP_TYPE_READBACK, // HOST_READBACK
};

MemoryType nri::GetMemoryType(D3D12_HEAP_TYPE heapType, D3D12_HEAP_FLAGS heapFlags) {
    return ((uint32_t)heapFlags) | ((uint32_t)heapType << 16);
}

MemoryType nri::GetMemoryType(MemoryLocation memoryLocation, const D3D12_RESOURCE_DESC& resourceDesc) {
    D3D12_HEAP_TYPE heapType = HEAP_TYPES[(uint32_t)memoryLocation];
    D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;

    // Required for Tier 1 resource heaps https://msdn.microsoft.com/en-us/library/windows/desktop/dn986743(v=vs.85).aspx
    if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    else if (resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET || resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
        heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
    else
        heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;

    return GetMemoryType(heapType, heapFlags);
}

D3D12_HEAP_TYPE nri::GetHeapType(MemoryType memoryType) {
    return (D3D12_HEAP_TYPE)(memoryType >> 16);
}

D3D12_HEAP_FLAGS nri::GetHeapFlags(MemoryType memoryType) {
    return (D3D12_HEAP_FLAGS)(memoryType & 0xffff);
}

bool nri::RequiresDedicatedAllocation(MemoryType memoryType) {
    D3D12_HEAP_FLAGS heapFlags = GetHeapFlags(memoryType);

    if ((heapFlags & D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES) == D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES)
        return true;

    return false;
}

D3D12_RESOURCE_FLAGS nri::GetBufferFlags(BufferUsageBits bufferUsageMask) {
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    if (bufferUsageMask & BufferUsageBits::SHADER_RESOURCE_STORAGE)
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    return flags;
};

D3D12_RESOURCE_FLAGS nri::GetTextureFlags(TextureUsageBits textureUsageMask) {
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    if (textureUsageMask & TextureUsageBits::SHADER_RESOURCE_STORAGE)
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    if (textureUsageMask & TextureUsageBits::COLOR_ATTACHMENT)
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    if (textureUsageMask & TextureUsageBits::DEPTH_STENCIL_ATTACHMENT) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        if (!(textureUsageMask & TextureUsageBits::SHADER_RESOURCE))
            flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }

    return flags;
};

D3D12_RESOURCE_DIMENSION nri::GetResourceDimension(TextureType textureType) {
    if (textureType == TextureType::TEXTURE_1D)
        return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    else if (textureType == TextureType::TEXTURE_2D)
        return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    else if (textureType == TextureType::TEXTURE_3D)
        return D3D12_RESOURCE_DIMENSION_TEXTURE3D;

    return D3D12_RESOURCE_DIMENSION_UNKNOWN;
}

constexpr std::array<D3D12_DESCRIPTOR_RANGE_TYPE, (uint32_t)DescriptorType::MAX_NUM> DESCRIPTOR_RANGE_TYPES = {
    D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, // SAMPLER
    D3D12_DESCRIPTOR_RANGE_TYPE_CBV,     // CONSTANT_BUFFER
    D3D12_DESCRIPTOR_RANGE_TYPE_SRV,     // TEXTURE
    D3D12_DESCRIPTOR_RANGE_TYPE_UAV,     // STORAGE_TEXTURE
    D3D12_DESCRIPTOR_RANGE_TYPE_SRV,     // BUFFER
    D3D12_DESCRIPTOR_RANGE_TYPE_UAV,     // STORAGE_BUFFER
    D3D12_DESCRIPTOR_RANGE_TYPE_SRV,     // STRUCTURED_BUFFER
    D3D12_DESCRIPTOR_RANGE_TYPE_UAV,     // STORAGE_STRUCTURED_BUFFER
    D3D12_DESCRIPTOR_RANGE_TYPE_SRV      // ACCELERATION_STRUCTURE
};

D3D12_DESCRIPTOR_RANGE_TYPE nri::GetDescriptorRangesType(DescriptorType descriptorType) {
    return DESCRIPTOR_RANGE_TYPES[(uint32_t)descriptorType];
}

D3D12_DESCRIPTOR_HEAP_TYPE nri::GetDescriptorHeapType(DescriptorType descriptorType) {
    return descriptorType == DescriptorType::SAMPLER ? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER : D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
}

D3D12_SHADER_VISIBILITY nri::GetShaderVisibility(StageBits shaderStages) {
    if (shaderStages == StageBits::ALL || shaderStages == StageBits::COMPUTE_SHADER || (shaderStages & StageBits::RAY_TRACING_SHADERS) != 0)
        return D3D12_SHADER_VISIBILITY_ALL;

    if (shaderStages == StageBits::VERTEX_SHADER)
        return D3D12_SHADER_VISIBILITY_VERTEX;

    if (shaderStages == StageBits::TESS_CONTROL_SHADER)
        return D3D12_SHADER_VISIBILITY_HULL;

    if (shaderStages == StageBits::TESS_EVALUATION_SHADER)
        return D3D12_SHADER_VISIBILITY_DOMAIN;

    if (shaderStages == StageBits::GEOMETRY_SHADER)
        return D3D12_SHADER_VISIBILITY_GEOMETRY;

    if (shaderStages == StageBits::FRAGMENT_SHADER)
        return D3D12_SHADER_VISIBILITY_PIXEL;

    if (shaderStages == StageBits::MESH_CONTROL_SHADER)
        return D3D12_SHADER_VISIBILITY_AMPLIFICATION;

    if (shaderStages == StageBits::MESH_EVALUATION_SHADER)
        return D3D12_SHADER_VISIBILITY_MESH;

    // Should be unreachable
    return D3D12_SHADER_VISIBILITY_ALL;
}

constexpr std::array<D3D12_PRIMITIVE_TOPOLOGY_TYPE, (uint32_t)Topology::MAX_NUM> PRIMITIVE_TOPOLOGY_TYPES = {
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,    // POINT_LIST
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,     // LINE_LIST
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,     // LINE_STRIP
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, // TRIANGLE_LIST
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, // TRIANGLE_STRIP
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,     // LINE_LIST_WITH_ADJACENCY
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,     // LINE_STRIP_WITH_ADJACENCY
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, // TRIANGLE_LIST_WITH_ADJACENCY
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, // TRIANGLE_STRIP_WITH_ADJACENCY
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH     // PATCH_LIST
};

D3D12_PRIMITIVE_TOPOLOGY_TYPE nri::GetPrimitiveTopologyType(Topology topology) {
    return PRIMITIVE_TOPOLOGY_TYPES[(uint32_t)topology];
}

constexpr std::array<D3D_PRIMITIVE_TOPOLOGY, 9> PRIMITIVE_TOPOLOGIES = {
    D3D_PRIMITIVE_TOPOLOGY_POINTLIST,        // POINT_LIST
    D3D_PRIMITIVE_TOPOLOGY_LINELIST,         // LINE_LIST
    D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,        // LINE_STRIP
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,     // TRIANGLE_LIST
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,    // TRIANGLE_STRIP
    D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ,     // LINE_LIST_WITH_ADJACENCY
    D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ,    // LINE_STRIP_WITH_ADJACENCY
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ, // TRIANGLE_LIST_WITH_ADJACENCY
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ // TRIANGLE_STRIP_WITH_ADJACENCY
};

D3D_PRIMITIVE_TOPOLOGY nri::GetPrimitiveTopology(Topology topology, uint8_t tessControlPointNum) {
    if (topology == Topology::PATCH_LIST)
        return (D3D_PRIMITIVE_TOPOLOGY)((uint8_t)D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ + tessControlPointNum);
    else
        return PRIMITIVE_TOPOLOGIES[(uint32_t)topology];
}

constexpr std::array<D3D12_FILL_MODE, (uint32_t)FillMode::MAX_NUM> FILL_MODES = {
    D3D12_FILL_MODE_SOLID,    // SOLID
    D3D12_FILL_MODE_WIREFRAME // WIREFRAME
};

D3D12_FILL_MODE nri::GetFillMode(FillMode fillMode) {
    return FILL_MODES[(uint32_t)fillMode];
}

constexpr std::array<D3D12_CULL_MODE, (uint32_t)CullMode::MAX_NUM> CULL_MODES = {
    D3D12_CULL_MODE_NONE,  // NONE
    D3D12_CULL_MODE_FRONT, // FRONT
    D3D12_CULL_MODE_BACK   // BACK
};

D3D12_CULL_MODE nri::GetCullMode(CullMode cullMode) {
    return CULL_MODES[(uint32_t)cullMode];
}

UINT8 nri::GetRenderTargetWriteMask(ColorWriteBits colorWriteMask) {
    return colorWriteMask & ColorWriteBits::RGBA;
}

constexpr std::array<D3D12_COMPARISON_FUNC, (uint32_t)CompareFunc::MAX_NUM> COMPARISON_FUNCS = {
    D3D12_COMPARISON_FUNC_NEVER,         // NONE
    D3D12_COMPARISON_FUNC_ALWAYS,        // ALWAYS
    D3D12_COMPARISON_FUNC_NEVER,         // NEVER
    D3D12_COMPARISON_FUNC_LESS,          // LESS
    D3D12_COMPARISON_FUNC_LESS_EQUAL,    // LESS_EQUAL
    D3D12_COMPARISON_FUNC_EQUAL,         // EQUAL
    D3D12_COMPARISON_FUNC_GREATER_EQUAL, // GREATER_EQUAL
    D3D12_COMPARISON_FUNC_GREATER,       // GREATER
    D3D12_COMPARISON_FUNC_NOT_EQUAL      // NOT_EQUAL
};

D3D12_COMPARISON_FUNC nri::GetComparisonFunc(CompareFunc compareFunc) {
    return COMPARISON_FUNCS[(uint32_t)compareFunc];
}

constexpr std::array<D3D12_STENCIL_OP, (uint32_t)StencilFunc::MAX_NUM> STENCIL_OPS = {
    D3D12_STENCIL_OP_KEEP,     // KEEP
    D3D12_STENCIL_OP_ZERO,     // ZERO
    D3D12_STENCIL_OP_REPLACE,  // REPLACE
    D3D12_STENCIL_OP_INCR_SAT, // INCREMENT_AND_CLAMP
    D3D12_STENCIL_OP_DECR_SAT, // DECREMENT_AND_CLAMP
    D3D12_STENCIL_OP_INVERT,   // INVERT
    D3D12_STENCIL_OP_INCR,     // INCREMENT_AND_WRAP
    D3D12_STENCIL_OP_DECR      // DECREMENT_AND_WRAP
};

D3D12_STENCIL_OP nri::GetStencilOp(StencilFunc stencilFunc) {
    return STENCIL_OPS[(uint32_t)stencilFunc];
}

constexpr std::array<D3D12_LOGIC_OP, (uint32_t)LogicFunc::MAX_NUM> LOGIC_OPS = {
    D3D12_LOGIC_OP_NOOP,          // NONE
    D3D12_LOGIC_OP_CLEAR,         // CLEAR
    D3D12_LOGIC_OP_AND,           // AND
    D3D12_LOGIC_OP_AND_REVERSE,   // AND_REVERSE
    D3D12_LOGIC_OP_COPY,          // COPY
    D3D12_LOGIC_OP_AND_INVERTED,  // AND_INVERTED
    D3D12_LOGIC_OP_XOR,           // XOR
    D3D12_LOGIC_OP_OR,            // OR
    D3D12_LOGIC_OP_NOR,           // NOR
    D3D12_LOGIC_OP_EQUIV,         // EQUIVALENT
    D3D12_LOGIC_OP_INVERT,        // INVERT
    D3D12_LOGIC_OP_OR_REVERSE,    // OR_REVERSE
    D3D12_LOGIC_OP_COPY_INVERTED, // COPY_INVERTED
    D3D12_LOGIC_OP_OR_INVERTED,   // OR_INVERTED
    D3D12_LOGIC_OP_NAND,          // NAND
    D3D12_LOGIC_OP_SET            // SET
};

D3D12_LOGIC_OP nri::GetLogicOp(LogicFunc logicFunc) {
    return LOGIC_OPS[(uint32_t)logicFunc];
}

constexpr std::array<D3D12_BLEND, (uint32_t)BlendFactor::MAX_NUM> BLENDS = {
    D3D12_BLEND_ZERO,             // ZERO
    D3D12_BLEND_ONE,              // ONE
    D3D12_BLEND_SRC_COLOR,        // SRC_COLOR
    D3D12_BLEND_INV_SRC_COLOR,    // ONE_MINUS_SRC_COLOR
    D3D12_BLEND_DEST_COLOR,       // DST_COLOR
    D3D12_BLEND_INV_DEST_COLOR,   // ONE_MINUS_DST_COLOR
    D3D12_BLEND_SRC_ALPHA,        // SRC_ALPHA
    D3D12_BLEND_INV_SRC_ALPHA,    // ONE_MINUS_SRC_ALPHA
    D3D12_BLEND_DEST_ALPHA,       // DST_ALPHA
    D3D12_BLEND_INV_DEST_ALPHA,   // ONE_MINUS_DST_ALPHA
    D3D12_BLEND_BLEND_FACTOR,     // CONSTANT_COLOR
    D3D12_BLEND_INV_BLEND_FACTOR, // ONE_MINUS_CONSTANT_COLOR
    D3D12_BLEND_BLEND_FACTOR,     // CONSTANT_ALPHA
    D3D12_BLEND_INV_BLEND_FACTOR, // ONE_MINUS_CONSTANT_ALPHA
    D3D12_BLEND_SRC_ALPHA_SAT,    // SRC_ALPHA_SATURATE
    D3D12_BLEND_SRC1_COLOR,       // SRC1_COLOR
    D3D12_BLEND_INV_SRC1_COLOR,   // ONE_MINUS_SRC1_COLOR
    D3D12_BLEND_SRC1_ALPHA,       // SRC1_ALPHA
    D3D12_BLEND_INV_SRC1_ALPHA    // ONE_MINUS_SRC1_ALPHA
};

D3D12_BLEND nri::GetBlend(BlendFactor blendFactor) {
    return BLENDS[(uint32_t)blendFactor];
}

constexpr std::array<D3D12_BLEND_OP, (uint32_t)BlendFunc::MAX_NUM> BLEND_OPS = {
    D3D12_BLEND_OP_ADD,          // ADD
    D3D12_BLEND_OP_SUBTRACT,     // SUBTRACT
    D3D12_BLEND_OP_REV_SUBTRACT, // REVERSE_SUBTRACT
    D3D12_BLEND_OP_MIN,          // MIN
    D3D12_BLEND_OP_MAX           // MAX
};

D3D12_BLEND_OP nri::GetBlendOp(BlendFunc blendFunc) {
    return BLEND_OPS[(uint32_t)blendFunc];
}

constexpr std::array<D3D12_TEXTURE_ADDRESS_MODE, (uint32_t)AddressMode::MAX_NUM> TEXTURE_ADDRESS_MODES = {
    D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // REPEAT
    D3D12_TEXTURE_ADDRESS_MODE_MIRROR, // MIRRORED_REPEAT
    D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // CLAMP_TO_EDGE
    D3D12_TEXTURE_ADDRESS_MODE_BORDER  // CLAMP_TO_BORDER
};

D3D12_TEXTURE_ADDRESS_MODE nri::GetAddressMode(AddressMode addressMode) {
    return TEXTURE_ADDRESS_MODES[(uint32_t)addressMode];
}

D3D12_FILTER nri::GetFilterIsotropic(Filter mip, Filter magnification, Filter minification, FilterExt filterExt, bool useComparison) {
    uint32_t combinedMask = 0;
    combinedMask |= mip == Filter::LINEAR ? 0x1 : 0;
    combinedMask |= magnification == Filter::LINEAR ? 0x4 : 0;
    combinedMask |= minification == Filter::LINEAR ? 0x10 : 0;

    if (useComparison)
        combinedMask |= 0x80;
    else if (filterExt == FilterExt::MIN)
        combinedMask |= 0x100;
    else if (filterExt == FilterExt::MAX)
        combinedMask |= 0x180;

    return (D3D12_FILTER)combinedMask;
}

D3D12_FILTER nri::GetFilterAnisotropic(FilterExt filterExt, bool useComparison) {
    if (filterExt == FilterExt::MIN)
        return D3D12_FILTER_MINIMUM_ANISOTROPIC;
    else if (filterExt == FilterExt::MAX)
        return D3D12_FILTER_MAXIMUM_ANISOTROPIC;

    return useComparison ? D3D12_FILTER_COMPARISON_ANISOTROPIC : D3D12_FILTER_ANISOTROPIC;
}

void nri::ConvertRects(D3D12_RECT* rectsD3D12, const Rect* rects, uint32_t rectNum) {
    for (uint32_t i = 0; i < rectNum; i++) {
        rectsD3D12[i].left = rects[i].x;
        rectsD3D12[i].top = rects[i].y;
        rectsD3D12[i].right = rects[i].x + rects[i].width;
        rectsD3D12[i].bottom = rects[i].y + rects[i].height;
    }
}

DXGI_FORMAT nri::GetShaderFormatForDepth(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_D16_UNORM:
            return DXGI_FORMAT_R16_UNORM;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        default:
            return format;
    }
}

uint64_t nri::GetMemorySizeD3D12(const MemoryD3D12Desc& memoryD3D12Desc) {
    return memoryD3D12Desc.d3d12Heap->GetDesc().SizeInBytes;
}

bool nri::GetTextureDesc(const TextureD3D12Desc& textureD3D12Desc, TextureDesc& textureDesc) {
    textureDesc = {};

    ID3D12Resource* resource = textureD3D12Desc.d3d12Resource;
    if (!resource)
        return false;

    D3D12_RESOURCE_DESC desc = resource->GetDesc();
    if (desc.Dimension < D3D12_RESOURCE_DIMENSION_TEXTURE1D)
        return false;

    textureDesc.type = (TextureType)(desc.Dimension - D3D12_RESOURCE_DIMENSION_TEXTURE1D);
    textureDesc.format = DXGIFormatToNRIFormat(desc.Format);
    textureDesc.width = (Dim_t)desc.Width;
    textureDesc.height = (Dim_t)desc.Height;
    textureDesc.depth = textureDesc.type == TextureType::TEXTURE_3D ? (Dim_t)desc.DepthOrArraySize : 1;
    textureDesc.mipNum = (Mip_t)desc.MipLevels;
    textureDesc.arraySize = textureDesc.type == TextureType::TEXTURE_3D ? 1 : (Dim_t)desc.DepthOrArraySize;
    textureDesc.sampleNum = (uint8_t)desc.SampleDesc.Count;
    textureDesc.nodeMask = ALL_NODES; // TODO: not in D3D12_RESOURCE_DESC...

    if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
        textureDesc.usageMask |= TextureUsageBits::COLOR_ATTACHMENT;
    if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
        textureDesc.usageMask |= TextureUsageBits::DEPTH_STENCIL_ATTACHMENT;
    if (!(desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))
        textureDesc.usageMask |= TextureUsageBits::SHADER_RESOURCE;
    if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
        textureDesc.usageMask |= TextureUsageBits::SHADER_RESOURCE_STORAGE;

    return true;
}

bool nri::GetBufferDesc(const BufferD3D12Desc& bufferD3D12Desc, BufferDesc& bufferDesc) {
    bufferDesc = {};

    ID3D12Resource* resource = bufferD3D12Desc.d3d12Resource;
    if (!resource)
        return false;

    D3D12_RESOURCE_DESC desc = resource->GetDesc();
    if (desc.Dimension < D3D12_RESOURCE_DIMENSION_TEXTURE1D)
        return false;

    bufferDesc.size = desc.Width;
    bufferDesc.structureStride = bufferD3D12Desc.structureStride;
    bufferDesc.nodeMask = ALL_NODES; // TODO: not in D3D12_RESOURCE_DESC...

    if (!(desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))
        bufferDesc.usageMask |= BufferUsageBits::SHADER_RESOURCE;
    if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
        bufferDesc.usageMask |= BufferUsageBits::SHADER_RESOURCE_STORAGE;

    return true;
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE nri::GetAccelerationStructureType(AccelerationStructureType accelerationStructureType) {
    static_assert(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL == (uint32_t)AccelerationStructureType::TOP_LEVEL, "Unsupported AccelerationStructureType.");
    static_assert(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL == (uint32_t)AccelerationStructureType::BOTTOM_LEVEL, "Unsupported AccelerationStructureType.");

    return (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE)accelerationStructureType;
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS nri::GetAccelerationStructureBuildFlags(AccelerationStructureBuildBits accelerationStructureBuildFlags) {
    static_assert(
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE == (uint32_t)AccelerationStructureBuildBits::ALLOW_UPDATE, "Unsupported AccelerationStructureBuildBits."
    );
    static_assert(
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION == (uint32_t)AccelerationStructureBuildBits::ALLOW_COMPACTION,
        "Unsupported AccelerationStructureBuildBits."
    );
    static_assert(
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE == (uint32_t)AccelerationStructureBuildBits::PREFER_FAST_TRACE,
        "Unsupported AccelerationStructureBuildBits."
    );
    static_assert(
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD == (uint32_t)AccelerationStructureBuildBits::PREFER_FAST_BUILD,
        "Unsupported AccelerationStructureBuildBits."
    );
    static_assert(
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY == (uint32_t)AccelerationStructureBuildBits::MINIMIZE_MEMORY,
        "Unsupported AccelerationStructureBuildBits."
    );

    return (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS)accelerationStructureBuildFlags;
}

D3D12_RAYTRACING_GEOMETRY_TYPE GetGeometryType(GeometryType geometryType) {
    static_assert(D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES == (uint32_t)GeometryType::TRIANGLES, "Unsupported GeometryType.");
    static_assert(D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS == (uint32_t)GeometryType::AABBS, "Unsupported GeometryType.");

    return (D3D12_RAYTRACING_GEOMETRY_TYPE)geometryType;
}

D3D12_RAYTRACING_GEOMETRY_FLAGS GetGeometryFlags(BottomLevelGeometryBits geometryFlagMask) {
    static_assert(D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE == (uint32_t)BottomLevelGeometryBits::OPAQUE_GEOMETRY, "Unsupported GeometryFlagMask.");
    static_assert(
        D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION == (uint32_t)BottomLevelGeometryBits::NO_DUPLICATE_ANY_HIT_INVOCATION, "Unsupported GeometryFlagMask."
    );

    return (D3D12_RAYTRACING_GEOMETRY_FLAGS)geometryFlagMask;
}

void nri::ConvertGeometryDescs(D3D12_RAYTRACING_GEOMETRY_DESC* geometryDescs, const GeometryObject* geometryObjects, uint32_t geometryObjectNum) {
    for (uint32_t i = 0; i < geometryObjectNum; i++) {
        geometryDescs[i].Type = GetGeometryType(geometryObjects[i].type);
        geometryDescs[i].Flags = GetGeometryFlags(geometryObjects[i].flags);

        if (geometryDescs[i].Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
            const Triangles& triangles = geometryObjects[i].triangles;
            geometryDescs[i].Triangles.Transform3x4 = triangles.transformBuffer ? ((BufferD3D12*)triangles.transformBuffer)->GetPointerGPU() + triangles.transformOffset : 0;
            geometryDescs[i].Triangles.IndexFormat = triangles.indexType == IndexType::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            geometryDescs[i].Triangles.VertexFormat = GetDxgiFormat(triangles.vertexFormat).typed;
            geometryDescs[i].Triangles.IndexCount = triangles.indexNum;
            geometryDescs[i].Triangles.VertexCount = triangles.vertexNum;
            geometryDescs[i].Triangles.IndexBuffer = triangles.indexBuffer ? ((BufferD3D12*)triangles.indexBuffer)->GetPointerGPU() + triangles.indexOffset : 0;
            geometryDescs[i].Triangles.VertexBuffer.StartAddress = ((BufferD3D12*)triangles.vertexBuffer)->GetPointerGPU() + triangles.vertexOffset;
            geometryDescs[i].Triangles.VertexBuffer.StrideInBytes = triangles.vertexStride;
        } else if (geometryDescs[i].Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
            const AABBs& aabbs = geometryObjects[i].boxes;
            geometryDescs[i].AABBs.AABBCount = aabbs.boxNum;
            geometryDescs[i].AABBs.AABBs.StartAddress = ((BufferD3D12*)aabbs.buffer)->GetPointerGPU() + aabbs.offset;
            geometryDescs[i].AABBs.AABBs.StrideInBytes = aabbs.stride;
        }
    }
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE nri::GetCopyMode(CopyMode copyMode) {
    static_assert(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE == (uint32_t)CopyMode::CLONE, "Unsupported CopyMode.");
    static_assert(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT == (uint32_t)CopyMode::COMPACT, "Unsupported CopyMode.");

    return (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE)copyMode;
}
