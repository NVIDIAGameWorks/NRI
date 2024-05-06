// © 2021 NVIDIA Corporation

#pragma once

#include "NRIMacro.h"

NRI_NAMESPACE_BEGIN

// Entities
NRI_FORWARD_STRUCT(Buffer);
NRI_FORWARD_STRUCT(CommandQueue);
NRI_FORWARD_STRUCT(CommandBuffer);
NRI_FORWARD_STRUCT(CommandAllocator);
NRI_FORWARD_STRUCT(Device);
NRI_FORWARD_STRUCT(Descriptor);
NRI_FORWARD_STRUCT(DescriptorPool);
NRI_FORWARD_STRUCT(DescriptorSet);
NRI_FORWARD_STRUCT(Fence);
NRI_FORWARD_STRUCT(Memory);
NRI_FORWARD_STRUCT(PipelineLayout);
NRI_FORWARD_STRUCT(Pipeline);
NRI_FORWARD_STRUCT(QueryPool);
NRI_FORWARD_STRUCT(Texture);

// Types
typedef uint32_t NRI_NAME(MemoryType);
typedef uint16_t NRI_NAME(Dim_t);
typedef uint8_t NRI_NAME(Mip_t);
typedef uint8_t NRI_NAME(Sample_t);

// Aliases
static const NRI_NAME(Dim_t) NRI_CONST_NAME(REMAINING_ARRAY_LAYERS) = 0; // only for "arraySize"
static const NRI_NAME(Mip_t) NRI_CONST_NAME(REMAINING_MIP_LEVELS) = 0; // only for "mipNum"
static const NRI_NAME(Dim_t) NRI_CONST_NAME(WHOLE_SIZE) = 0; // only for "Dim_t" and "size"
static const uint32_t NRI_CONST_NAME(ALL_SAMPLES) = 0; // only for "sampleMask"
static const uint32_t NRI_CONST_NAME(ONE_VIEWPORT) = 0; // only for "viewportNum"
static const bool NRI_CONST_NAME(VARIABLE_DESCRIPTOR_NUM) = true; // helper for "DescriptorRangeDesc::isDescriptorNumVariable"
static const bool NRI_CONST_NAME(DESCRIPTOR_ARRAY) = true; // helper for "DescriptorRangeDesc::isArray"
static const bool NRI_CONST_NAME(PARTIALLY_BOUND) = true; // helper for "DescriptorSetDesc::partiallyBound"

//===============================================================================================================================
// Common
//===============================================================================================================================
#pragma region [ Common ]

NRI_ENUM
(
    Result, uint8_t,

    SUCCESS,
    FAILURE,
    INVALID_ARGUMENT,
    OUT_OF_MEMORY,
    UNSUPPORTED,
    DEVICE_LOST,
    OUT_OF_DATE, // VK only: swap chain is out of date

    MAX_NUM
);

NRI_ENUM
(
    Vendor, uint8_t,

    UNKNOWN,
    NVIDIA,
    AMD,
    INTEL,

    MAX_NUM
);

NRI_ENUM
(
    GraphicsAPI, uint8_t,

    D3D11,
    D3D12,
    VULKAN,

    MAX_NUM
);

// left -> right : low -> high bits
// Expected (but not guaranteed) "FormatSupportBits" are provided, but "GetFormatSupport" should be used for querying real HW support
// To demote sRGB use the previous format, i.e. "format - 1"
NRI_ENUM
(
    //                                            STORAGE_BUFFER_ATOMICS
    //                                                  VERTEX_BUFFER  |
    //                                              STORAGE_BUFFER  |  |
    //                                                   BUFFER  |  |  |
    //                               STORAGE_TEXTURE_ATOMICS  |  |  |  |
    //                                              BLEND  |  |  |  |  |
    //                        DEPTH_STENCIL_ATTACHMENT  |  |  |  |  |  |
    //                             COLOR_ATTACHMENT  |  |  |  |  |  |  |
    //                           STORAGE_TEXTURE  |  |  |  |  |  |  |  |
    //                                TEXTURE  |  |  |  |  |  |  |  |  |
    //                                      |  |  |  |  |  |  |  |  |  |
    //                                      |    FormatSupportBits     |

    Format, uint8_t,

    UNKNOWN,                             // -  -  -  -  -  -  -  -  -  - 

    // Plain: 8 bits per channel
    R8_UNORM,                            // +  +  +  -  +  -  +  +  +  -
    R8_SNORM,                            // +  +  +  -  +  -  +  +  +  -
    R8_UINT,                             // +  +  +  -  -  -  +  +  +  -
    R8_SINT,                             // +  +  +  -  -  -  +  +  +  -

    RG8_UNORM,                           // +  +  +  -  +  -  +  +  +  -
    RG8_SNORM,                           // +  +  +  -  +  -  +  +  +  -
    RG8_UINT,                            // +  +  +  -  -  -  +  +  +  -
    RG8_SINT,                            // +  +  +  -  -  -  +  +  +  -

    BGRA8_UNORM,                         // +  +  +  -  +  -  +  +  +  -
    BGRA8_SRGB,                          // +  -  +  -  +  -  -  -  -  -

    RGBA8_UNORM,                         // +  +  +  -  +  -  +  +  +  -
    RGBA8_SRGB,                          // +  -  +  -  +  -  -  -  -  -
    RGBA8_SNORM,                         // +  +  +  -  +  -  +  +  +  -
    RGBA8_UINT,                          // +  +  +  -  -  -  +  +  +  -
    RGBA8_SINT,                          // +  +  +  -  -  -  +  +  +  -

    // Plain: 16 bits per channel
    R16_UNORM,                           // +  +  +  -  +  -  +  +  +  -
    R16_SNORM,                           // +  +  +  -  +  -  +  +  +  -
    R16_UINT,                            // +  +  +  -  -  -  +  +  +  -
    R16_SINT,                            // +  +  +  -  -  -  +  +  +  -
    R16_SFLOAT,                          // +  +  +  -  +  -  +  +  +  -

    RG16_UNORM,                          // +  +  +  -  +  -  +  +  +  -
    RG16_SNORM,                          // +  +  +  -  +  -  +  +  +  -
    RG16_UINT,                           // +  +  +  -  -  -  +  +  +  -
    RG16_SINT,                           // +  +  +  -  -  -  +  +  +  -
    RG16_SFLOAT,                         // +  +  +  -  +  -  +  +  +  -

    RGBA16_UNORM,                        // +  +  +  -  +  -  +  +  +  -
    RGBA16_SNORM,                        // +  +  +  -  +  -  +  +  +  -
    RGBA16_UINT,                         // +  +  +  -  -  -  +  +  +  -
    RGBA16_SINT,                         // +  +  +  -  -  -  +  +  +  -
    RGBA16_SFLOAT,                       // +  +  +  -  +  -  +  +  +  -

    // Plain: 32 bits per channel
    R32_UINT,                            // +  +  +  -  -  +  +  +  +  +
    R32_SINT,                            // +  +  +  -  -  +  +  +  +  +
    R32_SFLOAT,                          // +  +  +  -  +  +  +  +  +  +

    RG32_UINT,                           // +  +  +  -  -  -  +  +  +  -
    RG32_SINT,                           // +  +  +  -  -  -  +  +  +  -
    RG32_SFLOAT,                         // +  +  +  -  +  -  +  +  +  -

    RGB32_UINT,                          // +  -  -  -  -  -  +  -  +  -
    RGB32_SINT,                          // +  -  -  -  -  -  +  -  +  -
    RGB32_SFLOAT,                        // +  -  -  -  -  -  +  -  +  -

    RGBA32_UINT,                         // +  +  +  -  -  -  +  +  +  -
    RGBA32_SINT,                         // +  +  +  -  -  -  +  +  +  -
    RGBA32_SFLOAT,                       // +  +  +  -  +  -  +  +  +  -

    // Packed: 16 bits per pixel
    B5_G6_R5_UNORM,                      // +  -  +  -  +  -  -  -  -  -
    B5_G5_R5_A1_UNORM,                   // +  -  +  -  +  -  -  -  -  -
    B4_G4_R4_A4_UNORM,                   // +  -  +  -  +  -  -  -  -  -

    // Packed: 32 bits per pixel
    R10_G10_B10_A2_UNORM,                // +  +  +  -  +  -  +  +  +  -
    R10_G10_B10_A2_UINT,                 // +  +  +  -  -  -  +  +  +  -
    R11_G11_B10_UFLOAT,                  // +  +  +  -  +  -  +  +  +  -
    R9_G9_B9_E5_UFLOAT,                  // +  -  -  -  -  -  -  -  -  -

    // Block-compressed
    BC1_RGBA_UNORM,                      // +  -  -  -  -  -  -  -  -  -
    BC1_RGBA_SRGB,                       // +  -  -  -  -  -  -  -  -  -
    BC2_RGBA_UNORM,                      // +  -  -  -  -  -  -  -  -  -
    BC2_RGBA_SRGB,                       // +  -  -  -  -  -  -  -  -  -
    BC3_RGBA_UNORM,                      // +  -  -  -  -  -  -  -  -  -
    BC3_RGBA_SRGB,                       // +  -  -  -  -  -  -  -  -  -
    BC4_R_UNORM,                         // +  -  -  -  -  -  -  -  -  -
    BC4_R_SNORM,                         // +  -  -  -  -  -  -  -  -  -
    BC5_RG_UNORM,                        // +  -  -  -  -  -  -  -  -  -
    BC5_RG_SNORM,                        // +  -  -  -  -  -  -  -  -  -
    BC6H_RGB_UFLOAT,                     // +  -  -  -  -  -  -  -  -  -
    BC6H_RGB_SFLOAT,                     // +  -  -  -  -  -  -  -  -  -
    BC7_RGBA_UNORM,                      // +  -  -  -  -  -  -  -  -  -
    BC7_RGBA_SRGB,                       // +  -  -  -  -  -  -  -  -  -

    // Depth-stencil
    D16_UNORM,                           // -  -  -  +  -  -  -  -  -  -
    D24_UNORM_S8_UINT,                   // -  -  -  +  -  -  -  -  -  -
    D32_SFLOAT,                          // -  -  -  +  -  -  -  -  -  -
    D32_SFLOAT_S8_UINT_X24,              // -  -  -  +  -  -  -  -  -  -

    // Depth-stencil (SHADER_RESOURCE)
    R24_UNORM_X8,       // .x - depth    // +  -  -  -  -  -  -  -  -  -
    X24_G8_UINT,        // .y - stencil  // +  -  -  -  -  -  -  -  -  -
    R32_SFLOAT_X8_X24,  // .x - depth    // +  -  -  -  -  -  -  -  -  -
    X32_G8_UINT_X24,    // .y - stencil  // +  -  -  -  -  -  -  -  -  -

    MAX_NUM
);

NRI_ENUM_BITS
(
    FormatSupportBits, uint16_t,

    UNSUPPORTED                     = 0,

    // Texture
    TEXTURE                         = NRI_SET_BIT(0),
    STORAGE_TEXTURE                 = NRI_SET_BIT(1),
    COLOR_ATTACHMENT                = NRI_SET_BIT(2),
    DEPTH_STENCIL_ATTACHMENT        = NRI_SET_BIT(3),
    BLEND                           = NRI_SET_BIT(4),
    STORAGE_TEXTURE_ATOMICS         = NRI_SET_BIT(5),  // other than Load / Store

    // Buffer
    BUFFER                          = NRI_SET_BIT(6),
    STORAGE_BUFFER                  = NRI_SET_BIT(7),
    VERTEX_BUFFER                   = NRI_SET_BIT(8),
    STORAGE_BUFFER_ATOMICS          = NRI_SET_BIT(9)   // other than Load / Store
);

NRI_ENUM_BITS
(
    StageBits, uint32_t,

    // Special
    ALL                             = 0,               // Lazy default for barriers
    NONE                            = 0x7FFFFFFF,

    // Graphics                                        // Invoked by "CmdDraw*"
    INDEX_INPUT                     = NRI_SET_BIT(0),  //    Index buffer consumption
    VERTEX_SHADER                   = NRI_SET_BIT(1),  //    Vertex shader
    TESS_CONTROL_SHADER             = NRI_SET_BIT(2),  //    Tessellation control (hull) shader
    TESS_EVALUATION_SHADER          = NRI_SET_BIT(3),  //    Tessellation evaluation (domain) shader
    GEOMETRY_SHADER                 = NRI_SET_BIT(4),  //    Geometry shader
    MESH_CONTROL_SHADER             = NRI_SET_BIT(5),  //    Mesh control (task) shader
    MESH_EVALUATION_SHADER          = NRI_SET_BIT(6),  //    Mesh evaluation (amplification) shader
    FRAGMENT_SHADER                 = NRI_SET_BIT(7),  //    Fragment (pixel) shader
    DEPTH_STENCIL_ATTACHMENT        = NRI_SET_BIT(8),  //    Depth-stencil R/W operations
    COLOR_ATTACHMENT                = NRI_SET_BIT(9),  //    Color R/W operations

    // Compute                                         // Invoked by  "CmdDispatch*" (not Rays)
    COMPUTE_SHADER                  = NRI_SET_BIT(10), //    Compute shader

    // Ray tracing                                     // Invoked by "CmdDispatchRays"
    RAYGEN_SHADER                   = NRI_SET_BIT(11), //    Ray generation shader
    MISS_SHADER                     = NRI_SET_BIT(12), //    Miss shader
    INTERSECTION_SHADER             = NRI_SET_BIT(13), //    Intersection shader
    CLOSEST_HIT_SHADER              = NRI_SET_BIT(14), //    Closest hit shader
    ANY_HIT_SHADER                  = NRI_SET_BIT(15), //    Any hit shader
    CALLABLE_SHADER                 = NRI_SET_BIT(16), //    Callable shader

    // Other stages
    COPY                            = NRI_SET_BIT(17), // Invoked by "CmdCopy*", "CmdUpload*" and "CmdReadback*"
    CLEAR_STORAGE                   = NRI_SET_BIT(18), // Invoked by "CmdClearStorage*"
    ACCELERATION_STRUCTURE          = NRI_SET_BIT(19), // Invoked by "Cmd*AccelerationStructure*"

    // Modifiers
    INDIRECT                        = NRI_SET_BIT(20), // Invoked by "Indirect" command (used as addition to other bits)

    // Umbrella stages
    TESSELLATION_SHADERS            = NRI_ENUM_MEMBER_UNSCOPED(StageBits, TESS_CONTROL_SHADER) |
                                      NRI_ENUM_MEMBER_UNSCOPED(StageBits, TESS_EVALUATION_SHADER),

    MESH_SHADERS                    = NRI_ENUM_MEMBER_UNSCOPED(StageBits, MESH_CONTROL_SHADER) |
                                      NRI_ENUM_MEMBER_UNSCOPED(StageBits, MESH_EVALUATION_SHADER),

    GRAPHICS_SHADERS                = NRI_ENUM_MEMBER_UNSCOPED(StageBits, VERTEX_SHADER) |
                                      NRI_ENUM_MEMBER_UNSCOPED(StageBits, TESSELLATION_SHADERS) |
                                      NRI_ENUM_MEMBER_UNSCOPED(StageBits, GEOMETRY_SHADER) |
                                      NRI_ENUM_MEMBER_UNSCOPED(StageBits, MESH_SHADERS) |
                                      NRI_ENUM_MEMBER_UNSCOPED(StageBits, FRAGMENT_SHADER),

    // Invoked by "CmdDispatchRays"
    RAY_TRACING_SHADERS             = NRI_ENUM_MEMBER_UNSCOPED(StageBits, RAYGEN_SHADER) |
                                      NRI_ENUM_MEMBER_UNSCOPED(StageBits, MISS_SHADER) |
                                      NRI_ENUM_MEMBER_UNSCOPED(StageBits, INTERSECTION_SHADER) |
                                      NRI_ENUM_MEMBER_UNSCOPED(StageBits, CLOSEST_HIT_SHADER) |
                                      NRI_ENUM_MEMBER_UNSCOPED(StageBits, ANY_HIT_SHADER) |
                                      NRI_ENUM_MEMBER_UNSCOPED(StageBits, CALLABLE_SHADER),

    // Invoked by "CmdDraw*"
    DRAW                            = NRI_ENUM_MEMBER_UNSCOPED(StageBits, INDEX_INPUT) |
                                      NRI_ENUM_MEMBER_UNSCOPED(StageBits, GRAPHICS_SHADERS) |
                                      NRI_ENUM_MEMBER_UNSCOPED(StageBits, DEPTH_STENCIL_ATTACHMENT) |
                                      NRI_ENUM_MEMBER_UNSCOPED(StageBits, COLOR_ATTACHMENT)
);

NRI_STRUCT(Rect)
{
    int16_t x;
    int16_t y;
    NRI_NAME(Dim_t) width;
    NRI_NAME(Dim_t) height;
};

NRI_STRUCT(Viewport)
{
    float x;
    float y;
    float width;
    float height;
    float depthRangeMin;
    float depthRangeMax;
};

NRI_STRUCT(Color32f)
{
    float x, y, z, w;
};

NRI_STRUCT(Color32ui)
{
    uint32_t x, y, z, w;
};

NRI_STRUCT(Color32i)
{
    int32_t x, y, z, w;
};

NRI_STRUCT(DepthStencil)
{
    float depth;
    uint8_t stencil;
};

NRI_STRUCT(SamplePosition)
{
    int8_t x, y; // [-8; 7]
};

#pragma endregion

//===============================================================================================================================
// Creation
//===============================================================================================================================
#pragma region [ Creation ]

NRI_ENUM
(
    CommandQueueType, uint8_t,

    GRAPHICS,
    COMPUTE,
    COPY,

    MAX_NUM
);

NRI_ENUM
(
    MemoryLocation, uint8_t,

    DEVICE,
    DEVICE_UPLOAD, // soft fallback to HOST_UPLOAD
    HOST_UPLOAD,
    HOST_READBACK,

    MAX_NUM
);

NRI_ENUM
(
    TextureType, uint8_t,

    TEXTURE_1D,
    TEXTURE_2D,
    TEXTURE_3D,

    MAX_NUM
);

NRI_ENUM
(
    Texture1DViewType, uint8_t,

    SHADER_RESOURCE_1D,
    SHADER_RESOURCE_1D_ARRAY,
    SHADER_RESOURCE_STORAGE_1D,
    SHADER_RESOURCE_STORAGE_1D_ARRAY,
    COLOR_ATTACHMENT,
    DEPTH_STENCIL_ATTACHMENT,

    MAX_NUM
);

NRI_ENUM
(
    Texture2DViewType, uint8_t,

    SHADER_RESOURCE_2D,
    SHADER_RESOURCE_2D_ARRAY,
    SHADER_RESOURCE_CUBE,
    SHADER_RESOURCE_CUBE_ARRAY,
    SHADER_RESOURCE_STORAGE_2D,
    SHADER_RESOURCE_STORAGE_2D_ARRAY,
    COLOR_ATTACHMENT,
    DEPTH_STENCIL_ATTACHMENT,

    MAX_NUM
);

NRI_ENUM
(
    Texture3DViewType, uint8_t,

    SHADER_RESOURCE_3D,
    SHADER_RESOURCE_STORAGE_3D,
    COLOR_ATTACHMENT,

    MAX_NUM
);

NRI_ENUM
(
    BufferViewType, uint8_t,

    SHADER_RESOURCE,
    SHADER_RESOURCE_STORAGE,
    CONSTANT,

    MAX_NUM
);

NRI_ENUM
(
    DescriptorType, uint8_t,

    SAMPLER,
    CONSTANT_BUFFER,
    TEXTURE,
    STORAGE_TEXTURE,
    BUFFER,
    STORAGE_BUFFER,
    STRUCTURED_BUFFER,
    STORAGE_STRUCTURED_BUFFER,
    ACCELERATION_STRUCTURE,

    MAX_NUM
);

NRI_ENUM_BITS
(
    TextureUsageBits, uint8_t,

    NONE                                = 0,
    SHADER_RESOURCE                     = NRI_SET_BIT(0),
    SHADER_RESOURCE_STORAGE             = NRI_SET_BIT(1),
    COLOR_ATTACHMENT                    = NRI_SET_BIT(2),
    DEPTH_STENCIL_ATTACHMENT            = NRI_SET_BIT(3)
);

NRI_ENUM_BITS
(
    BufferUsageBits, uint8_t,

    NONE                                = 0,
    SHADER_RESOURCE                     = NRI_SET_BIT(0),
    SHADER_RESOURCE_STORAGE             = NRI_SET_BIT(1),
    VERTEX_BUFFER                       = NRI_SET_BIT(2),
    INDEX_BUFFER                        = NRI_SET_BIT(3),
    CONSTANT_BUFFER                     = NRI_SET_BIT(4),
    ARGUMENT_BUFFER                     = NRI_SET_BIT(5),
    RAY_TRACING_BUFFER                  = NRI_SET_BIT(6),
    ACCELERATION_STRUCTURE_BUILD_READ   = NRI_SET_BIT(7)
);

NRI_ENUM_BITS
(
    ResourceViewBits, uint8_t,

    READONLY_DEPTH                      = NRI_SET_BIT(0),
    READONLY_STENCIL                    = NRI_SET_BIT(1)
);

// Resources
NRI_STRUCT(TextureDesc)
{
    NRI_NAME(TextureType) type;
    NRI_NAME(TextureUsageBits) usageMask;
    NRI_NAME(Format) format;
    NRI_NAME(Dim_t) width;
    NRI_NAME(Dim_t) height;
    NRI_NAME(Dim_t) depth;
    NRI_NAME(Mip_t) mipNum;
    NRI_NAME(Dim_t) arraySize;
    NRI_NAME(Sample_t) sampleNum;
};

NRI_STRUCT(BufferDesc)
{
    uint64_t size;
    uint32_t structureStride;
    NRI_NAME(BufferUsageBits) usageMask;
};

// Descriptors (Views)
NRI_STRUCT(Texture1DViewDesc)
{
    const NRI_NAME(Texture)* texture;
    NRI_NAME(Texture1DViewType) viewType;
    NRI_NAME(Format) format;
    NRI_NAME(Mip_t) mipOffset;
    NRI_NAME(Mip_t) mipNum;
    NRI_NAME(Dim_t) arrayOffset;
    NRI_NAME(Dim_t) arraySize;
    NRI_NAME(ResourceViewBits) flags;
};

NRI_STRUCT(Texture2DViewDesc)
{
    const NRI_NAME(Texture)* texture;
    NRI_NAME(Texture2DViewType) viewType;
    NRI_NAME(Format) format;
    NRI_NAME(Mip_t) mipOffset;
    NRI_NAME(Mip_t) mipNum;
    NRI_NAME(Dim_t) arrayOffset;
    NRI_NAME(Dim_t) arraySize;
    NRI_NAME(ResourceViewBits) flags;
};

NRI_STRUCT(Texture3DViewDesc)
{
    const NRI_NAME(Texture)* texture;
    NRI_NAME(Texture3DViewType) viewType;
    NRI_NAME(Format) format;
    NRI_NAME(Mip_t) mipOffset;
    NRI_NAME(Mip_t) mipNum;
    NRI_NAME(Dim_t) sliceOffset;
    NRI_NAME(Dim_t) sliceNum;
    NRI_NAME(ResourceViewBits) flags;
};

NRI_STRUCT(BufferViewDesc)
{
    const NRI_NAME(Buffer)* buffer;
    NRI_NAME(BufferViewType) viewType;
    NRI_NAME(Format) format;
    uint64_t offset;
    uint64_t size;
};

// Descriptor pool
NRI_STRUCT(DescriptorPoolDesc)
{
    uint32_t descriptorSetMaxNum;
    uint32_t samplerMaxNum;
    uint32_t constantBufferMaxNum;
    uint32_t dynamicConstantBufferMaxNum;
    uint32_t textureMaxNum;
    uint32_t storageTextureMaxNum;
    uint32_t bufferMaxNum;
    uint32_t storageBufferMaxNum;
    uint32_t structuredBufferMaxNum;
    uint32_t storageStructuredBufferMaxNum;
    uint32_t accelerationStructureMaxNum;
};

#pragma endregion

//===============================================================================================================================
// Pipeline layout
//===============================================================================================================================
#pragma region [ Pipeline layout ]

NRI_STRUCT(PushConstantDesc)
{
    uint32_t registerIndex;
    uint32_t size;
    NRI_NAME(StageBits) shaderStages;
};

NRI_STRUCT(DescriptorRangeDesc)
{
    uint32_t baseRegisterIndex;
    uint32_t descriptorNum;
    NRI_NAME(DescriptorType) descriptorType;
    NRI_NAME(StageBits) shaderStages;
    bool isDescriptorNumVariable;
    bool isArray;
};

NRI_STRUCT(DynamicConstantBufferDesc)
{
    uint32_t registerIndex;
    NRI_NAME(StageBits) shaderStages;
};

NRI_STRUCT(DescriptorSetDesc)
{
    uint32_t registerSpace;
    const NRI_NAME(DescriptorRangeDesc)* ranges;
    uint32_t rangeNum;
    const NRI_NAME(DynamicConstantBufferDesc)* dynamicConstantBuffers;
    uint32_t dynamicConstantBufferNum;
    bool partiallyBound;
};

NRI_STRUCT(PipelineLayoutDesc)
{
    const NRI_NAME(DescriptorSetDesc)* descriptorSets;
    const NRI_NAME(PushConstantDesc)* pushConstants;
    uint32_t descriptorSetNum;
    uint32_t pushConstantNum;
    NRI_NAME(StageBits) shaderStages;
    bool ignoreGlobalSPIRVOffsets;
    bool enableD3D12DrawParametersEmulation; // implicitly expects "enableD3D12DrawParametersEmulation" passed during device creation
};

#pragma endregion

//===============================================================================================================================
// Input assembly
//===============================================================================================================================
#pragma region [ Input assembly ]

NRI_ENUM
(
    VertexStreamStepRate, uint8_t,

    PER_VERTEX,
    PER_INSTANCE,

    MAX_NUM
);

NRI_ENUM
(
    IndexType, uint8_t,

    UINT16,
    UINT32,

    MAX_NUM
);

NRI_ENUM
(
    PrimitiveRestart, uint8_t,

    DISABLED,
    INDICES_UINT16,
    INDICES_UINT32,

    MAX_NUM
);

NRI_ENUM
(
    Topology, uint8_t,

    POINT_LIST,
    LINE_LIST,
    LINE_STRIP,
    TRIANGLE_LIST,
    TRIANGLE_STRIP,
    LINE_LIST_WITH_ADJACENCY,
    LINE_STRIP_WITH_ADJACENCY,
    TRIANGLE_LIST_WITH_ADJACENCY,
    TRIANGLE_STRIP_WITH_ADJACENCY,
    PATCH_LIST,

    MAX_NUM
);

NRI_STRUCT(InputAssemblyDesc)
{
    NRI_NAME(Topology) topology;
    uint8_t tessControlPointNum;
    NRI_NAME(PrimitiveRestart) primitiveRestart;
};

NRI_STRUCT(VertexAttributeD3D)
{
    const char* semanticName;
    uint32_t semanticIndex;
};

NRI_STRUCT(VertexAttributeVK)
{
    uint32_t location;
};

NRI_STRUCT(VertexAttributeDesc)
{
    NRI_NAME(VertexAttributeD3D) d3d;
    NRI_NAME(VertexAttributeVK) vk;
    uint32_t offset;
    NRI_NAME(Format) format;
    uint16_t streamIndex;
};

NRI_STRUCT(VertexStreamDesc)
{
    uint16_t stride;
    uint16_t bindingSlot;
    NRI_NAME(VertexStreamStepRate) stepRate;
};

NRI_STRUCT(VertexInputDesc)
{
    const NRI_NAME(VertexAttributeDesc)* attributes;
    const NRI_NAME(VertexStreamDesc)* streams;
    uint8_t attributeNum;
    uint8_t streamNum;
};

#pragma endregion

//===============================================================================================================================
// Rasterization
//===============================================================================================================================
#pragma region [ Rasterization ]

NRI_ENUM
(
    FillMode, uint8_t,

    SOLID,
    WIREFRAME,

    MAX_NUM
);

NRI_ENUM
(
    CullMode, uint8_t,

    NONE,
    FRONT,
    BACK,

    MAX_NUM
);

NRI_STRUCT(RasterizationDesc)
{
    uint32_t viewportNum;
    float depthBias;
    float depthBiasClamp;
    float depthBiasSlopeFactor;
    NRI_NAME(FillMode) fillMode;
    NRI_NAME(CullMode) cullMode;
    bool frontCounterClockwise;
    bool depthClamp;
    bool antialiasedLines; // Requires "isLineSmoothingSupported"
    bool conservativeRasterization; // Requires "conservativeRasterTier > 0"
};

NRI_STRUCT(MultisampleDesc)
{
    uint32_t sampleMask;
    NRI_NAME(Sample_t) sampleNum;
    bool alphaToCoverage;
    bool programmableSampleLocations; // Requires "isProgrammableSampleLocationsSupported"
};

#pragma endregion

//===============================================================================================================================
// Output merger
//===============================================================================================================================
#pragma region [ Output merger ]

NRI_ENUM
(
    LogicFunc, uint8_t,

    NONE,

    // Requires "isLogicOpSupported"
    // S - source color 0
    // D - destination color

    CLEAR,                      // 0
    AND,                        // S & D
    AND_REVERSE,                // S & ~D
    COPY,                       // S
    AND_INVERTED,               // ~S & D
    XOR,                        // S ^ D
    OR,                         // S | D
    NOR,                        // ~(S | D)
    EQUIVALENT,                 // ~(S ^ D)
    INVERT,                     // ~D
    OR_REVERSE,                 // S | ~D
    COPY_INVERTED,              // ~S
    OR_INVERTED,                // ~S | D
    NAND,                       // ~(S & D)
    SET,                        // 1

    MAX_NUM
);

NRI_ENUM
(
    CompareFunc, uint8_t,

    // R - fragment's depth or stencil reference
    // D - depth or stencil buffer

    NONE,                       // test is disabled
    ALWAYS,                     // true
    NEVER,                      // false
    EQUAL,                      // R == D
    NOT_EQUAL,                  // R != D
    LESS,                       // R < D
    LESS_EQUAL,                 // R <= D
    GREATER,                    // R > D
    GREATER_EQUAL,              // R >= D

    MAX_NUM
);

NRI_ENUM
(
    StencilFunc, uint8_t,

    // R - reference, set by "CmdSetStencilReference"
    // D - stencil buffer

    KEEP,                       // D = D
    ZERO,                       // D = 0
    REPLACE,                    // D = R
    INCREMENT_AND_CLAMP,        // D = min(D++, 255)
    DECREMENT_AND_CLAMP,        // D = max(D--, 0)
    INVERT,                     // D = ~D
    INCREMENT_AND_WRAP,         // D++
    DECREMENT_AND_WRAP,         // D--

    MAX_NUM
);

NRI_ENUM
(
    BlendFactor, uint8_t,

    // S0 - source color 0
    // S1 - source color 1
    // D - destination color
    // C - blend constants (CmdSetBlendConstants)

                                // RGB                               ALPHA
    ZERO,                       // 0                                 0
    ONE,                        // 1                                 1
    SRC_COLOR,                  // S0.r, S0.g, S0.b                  S0.a
    ONE_MINUS_SRC_COLOR,        // 1 - S0.r, 1 - S0.g, 1 - S0.b      1 - S0.a
    DST_COLOR,                  // D.r, D.g, D.b                     D.a
    ONE_MINUS_DST_COLOR,        // 1 - D.r, 1 - D.g, 1 - D.b         1 - D.a
    SRC_ALPHA,                  // S0.a                              S0.a
    ONE_MINUS_SRC_ALPHA,        // 1 - S0.a                          1 - S0.a
    DST_ALPHA,                  // D.a                               D.a
    ONE_MINUS_DST_ALPHA,        // 1 - D.a                           1 - D.a
    CONSTANT_COLOR,             // C.r, C.g, C.b                     C.a
    ONE_MINUS_CONSTANT_COLOR,   // 1 - C.r, 1 - C.g, 1 - C.b         1 - C.a
    CONSTANT_ALPHA,             // C.a                               C.a
    ONE_MINUS_CONSTANT_ALPHA,   // 1 - C.a                           1 - C.a
    SRC_ALPHA_SATURATE,         // min(S0.a, 1 - D.a)                1
    SRC1_COLOR,                 // S1.r, S1.g, S1.b                  S1.a
    ONE_MINUS_SRC1_COLOR,       // 1 - S1.r, 1 - S1.g, 1 - S1.b      1 - S1.a
    SRC1_ALPHA,                 // S1.a                              S1.a
    ONE_MINUS_SRC1_ALPHA,       // 1 - S1.a                          1 - S1.a

    MAX_NUM
);

NRI_ENUM
(
    BlendFunc, uint8_t,

    // S - source color
    // D - destination color
    // Sf - source factor, produced by BlendFactor
    // Df - destination factor, produced by BlendFactor

    ADD,                        // S * Sf + D * Df
    SUBTRACT,                   // S * Sf - D * Df
    REVERSE_SUBTRACT,           // D * Df - S * Sf
    MIN,                        // min(S, D)
    MAX,                        // max(S, D)

    MAX_NUM
);

NRI_ENUM
(
    AttachmentContentType, uint8_t,

    COLOR,
    DEPTH,
    STENCIL,
    DEPTH_STENCIL,

    MAX_NUM
);

NRI_ENUM_BITS
(
    ColorWriteBits, uint8_t,

    R    = NRI_SET_BIT(0),
    G    = NRI_SET_BIT(1),
    B    = NRI_SET_BIT(2),
    A    = NRI_SET_BIT(3),

    RGB  = NRI_ENUM_MEMBER_UNSCOPED(ColorWriteBits, R) |
           NRI_ENUM_MEMBER_UNSCOPED(ColorWriteBits, G) |
           NRI_ENUM_MEMBER_UNSCOPED(ColorWriteBits, B),

    RGBA = NRI_ENUM_MEMBER_UNSCOPED(ColorWriteBits, RGB) |
           NRI_ENUM_MEMBER_UNSCOPED(ColorWriteBits, A)
);

NRI_UNION(ClearValue)
{
    NRI_NAME(DepthStencil) depthStencil;
    NRI_NAME(Color32f) color32f;
    NRI_NAME(Color32ui) color32ui;
    NRI_NAME(Color32i) color32i;
};

NRI_STRUCT(ClearDesc)
{
    NRI_NAME(ClearValue) value;
    NRI_NAME(AttachmentContentType) attachmentContentType;
    uint32_t colorAttachmentIndex;
};

NRI_STRUCT(StencilDesc)
{
    NRI_NAME(CompareFunc) compareFunc; // compareFunc != NONE, expects "CmdSetStencilReference"
    NRI_NAME(StencilFunc) fail;
    NRI_NAME(StencilFunc) pass;
    NRI_NAME(StencilFunc) depthFail;
    uint8_t writeMask;
    uint8_t compareMask;
};

NRI_STRUCT(BlendingDesc)
{
    NRI_NAME(BlendFactor) srcFactor;
    NRI_NAME(BlendFactor) dstFactor;
    NRI_NAME(BlendFunc) func;
};

NRI_STRUCT(ColorAttachmentDesc)
{
    NRI_NAME(Format) format;
    NRI_NAME(BlendingDesc) colorBlend;
    NRI_NAME(BlendingDesc) alphaBlend;
    NRI_NAME(ColorWriteBits) colorWriteMask;
    bool blendEnabled;
};

NRI_STRUCT(DepthAttachmentDesc)
{
    NRI_NAME(CompareFunc) compareFunc;
    bool write;
    bool boundsTest; // Requires "isDepthBoundsTestSupported", expects "CmdSetDepthBounds"
};

NRI_STRUCT(StencilAttachmentDesc)
{
    NRI_NAME(StencilDesc) front;
    NRI_NAME(StencilDesc) back; // "back.writeMask" requires "isIndependentFrontAndBackStencilReferenceAndMasksSupported"
};

NRI_STRUCT(OutputMergerDesc)
{
    const NRI_NAME(ColorAttachmentDesc)* color;
    NRI_NAME(DepthAttachmentDesc) depth;
    NRI_NAME(StencilAttachmentDesc) stencil;
    NRI_NAME(Format) depthStencilFormat;
    NRI_NAME(LogicFunc) colorLogicFunc;
    uint32_t colorNum;
};

NRI_STRUCT(AttachmentsDesc)
{
    const NRI_NAME(Descriptor)* depthStencil;
    const NRI_NAME(Descriptor)* const* colors;
    uint32_t colorNum;
};

#pragma endregion

//===============================================================================================================================
// Sampler
//===============================================================================================================================
#pragma region [ Sampler ]

NRI_ENUM
(
    BorderColor, uint8_t,

    FLOAT_TRANSPARENT_BLACK,
    FLOAT_OPAQUE_BLACK,
    FLOAT_OPAQUE_WHITE,
    INT_TRANSPARENT_BLACK,
    INT_OPAQUE_BLACK,
    INT_OPAQUE_WHITE,

    MAX_NUM
);

NRI_ENUM
(
    Filter, uint8_t,

    NEAREST,
    LINEAR,

    MAX_NUM
);

NRI_ENUM
(
    FilterExt, uint8_t,

    NONE,

    // Requires "isTextureFilterMinMaxSupported"
    MIN,
    MAX,

    MAX_NUM
);

NRI_ENUM
(
    AddressMode, uint8_t,

    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER,

    MAX_NUM
);

NRI_STRUCT(AddressModes)
{
    NRI_NAME(AddressMode) u, v, w;
};

NRI_STRUCT(Filters)
{
    NRI_NAME(Filter) min, mag, mip;
    NRI_NAME(FilterExt) ext;
};

NRI_STRUCT(SamplerDesc)
{
    NRI_NAME(Filters) filters;
    uint8_t anisotropy;
    float mipBias;
    float mipMin;
    float mipMax;
    NRI_NAME(AddressModes) addressModes;
    NRI_NAME(CompareFunc) compareFunc;
    NRI_NAME(BorderColor) borderColor;
};

#pragma endregion

//===============================================================================================================================
// Pipeline
//===============================================================================================================================
#pragma region [ Pipeline ]

NRI_STRUCT(ShaderDesc)
{
    NRI_NAME(StageBits) stage;
    const void* bytecode;
    uint64_t size;
    const char* entryPointName;
};

NRI_STRUCT(GraphicsPipelineDesc)
{
    const NRI_NAME(PipelineLayout)* pipelineLayout;
    const NRI_NAME(VertexInputDesc)* vertexInput; // optional
    NRI_NAME(InputAssemblyDesc) inputAssembly;
    NRI_NAME(RasterizationDesc) rasterization;
    const NRI_NAME(MultisampleDesc)* multisample; // optional
    NRI_NAME(OutputMergerDesc) outputMerger;
    const NRI_NAME(ShaderDesc)* shaders;
    uint32_t shaderNum;
};

NRI_STRUCT(ComputePipelineDesc)
{
    const NRI_NAME(PipelineLayout)* pipelineLayout;
    NRI_NAME(ShaderDesc) shader;
};

#pragma endregion

//===============================================================================================================================
// Barrier
//===============================================================================================================================
#pragma region [ Barrier ]

NRI_ENUM
(
    Layout, uint8_t,

    UNKNOWN,
    COLOR_ATTACHMENT,
    DEPTH_STENCIL,
    DEPTH_STENCIL_READONLY,
    SHADER_RESOURCE,
    SHADER_RESOURCE_STORAGE,
    COPY_SOURCE,
    COPY_DESTINATION,
    PRESENT,

    MAX_NUM
);

NRI_ENUM_BITS
(
    AccessBits, uint16_t,                           // Compatible "StageBits" (including ALL):

    UNKNOWN                      = 0,
    INDEX_BUFFER                 = NRI_SET_BIT(0),  // INDEX_INPUT
    VERTEX_BUFFER                = NRI_SET_BIT(1),  // VERTEX_SHADER
    CONSTANT_BUFFER              = NRI_SET_BIT(2),  // GRAPHICS_SHADERS, COMPUTE_SHADER, RAY_TRACING_SHADERS
    SHADER_RESOURCE              = NRI_SET_BIT(3),  // GRAPHICS_SHADERS, COMPUTE_SHADER, RAY_TRACING_SHADERS
    SHADER_RESOURCE_STORAGE      = NRI_SET_BIT(4),  // GRAPHICS_SHADERS, COMPUTE_SHADER, RAY_TRACING_SHADERS, CLEAR_STORAGE
    ARGUMENT_BUFFER              = NRI_SET_BIT(5),  // INDIRECT
    COLOR_ATTACHMENT             = NRI_SET_BIT(6),  // COLOR_ATTACHMENT
    DEPTH_STENCIL_WRITE          = NRI_SET_BIT(7),  // DEPTH_STENCIL_ATTACHMENT
    DEPTH_STENCIL_READ           = NRI_SET_BIT(8),  // DEPTH_STENCIL_ATTACHMENT
    COPY_SOURCE                  = NRI_SET_BIT(9),  // COPY
    COPY_DESTINATION             = NRI_SET_BIT(10), // COPY
    ACCELERATION_STRUCTURE_READ  = NRI_SET_BIT(11), // COMPUTE_SHADER, RAY_TRACING, ACCELERATION_STRUCTURE
    ACCELERATION_STRUCTURE_WRITE = NRI_SET_BIT(12), // COMPUTE_SHADER, RAY_TRACING, ACCELERATION_STRUCTURE
    SHADING_RATE                 = NRI_SET_BIT(13)  // FRAGMENT_SHADER // TODO: WIP
);

NRI_STRUCT(AccessStage)
{
    NRI_NAME(AccessBits) access;
    NRI_NAME(StageBits) stages;
};

NRI_STRUCT(AccessLayoutStage)
{
    NRI_NAME(AccessBits) access;
    NRI_NAME(Layout) layout;
    NRI_NAME(StageBits) stages;
};

NRI_STRUCT(GlobalBarrierDesc)
{
    NRI_NAME(AccessStage) before;
    NRI_NAME(AccessStage) after;
};

NRI_STRUCT(BufferBarrierDesc)
{
    NRI_NAME(Buffer)* buffer;
    NRI_NAME(AccessStage) before;
    NRI_NAME(AccessStage) after;
};

NRI_STRUCT(TextureBarrierDesc)
{
    NRI_NAME(Texture)* texture;
    NRI_NAME(AccessLayoutStage) before;
    NRI_NAME(AccessLayoutStage) after;
    NRI_NAME(Mip_t) mipOffset;
    NRI_NAME(Mip_t) mipNum;
    NRI_NAME(Dim_t) arrayOffset;
    NRI_NAME(Dim_t) arraySize;
};

NRI_STRUCT(BarrierGroupDesc)
{
    const NRI_NAME(GlobalBarrierDesc)* globals;
    const NRI_NAME(BufferBarrierDesc)* buffers;
    const NRI_NAME(TextureBarrierDesc)* textures;
    uint16_t globalNum;
    uint16_t bufferNum;
    uint16_t textureNum;
};

#pragma endregion

//===============================================================================================================================
// Other
//===============================================================================================================================
#pragma region [ Other ]

// Copy
NRI_STRUCT(TextureRegionDesc)
{
    uint16_t x;
    uint16_t y;
    uint16_t z;
    NRI_NAME(Dim_t) width;
    NRI_NAME(Dim_t) height;
    NRI_NAME(Dim_t) depth;
    NRI_NAME(Mip_t) mipOffset;
    NRI_NAME(Dim_t) arrayOffset;
};

NRI_STRUCT(TextureDataLayoutDesc)
{
    uint64_t offset;
    uint32_t rowPitch;
    uint32_t slicePitch;
};

// Work submission
NRI_STRUCT(FenceSubmitDesc)
{
    NRI_NAME(Fence)* fence;
    uint64_t value;
    NRI_NAME(StageBits) stages;
};

NRI_STRUCT(QueueSubmitDesc)
{
    const NRI_NAME(FenceSubmitDesc)* waitFences;
    uint32_t waitFenceNum;
    const NRI_NAME(CommandBuffer)* const* commandBuffers;
    uint32_t commandBufferNum;
    const NRI_NAME(FenceSubmitDesc)* signalFences;
    uint32_t signalFenceNum;
};

// Memory
NRI_STRUCT(BufferMemoryBindingDesc)
{
    NRI_NAME(Memory)* memory;
    NRI_NAME(Buffer)* buffer;
    uint64_t offset;
};

NRI_STRUCT(TextureMemoryBindingDesc)
{
    NRI_NAME(Memory)* memory;
    NRI_NAME(Texture)* texture;
    uint64_t offset;
};

NRI_STRUCT(MemoryDesc)
{
    uint64_t size;
    uint32_t alignment;
    NRI_NAME(MemoryType) type;
    bool mustBeDedicated;
};

// Clear storage
NRI_STRUCT(ClearStorageBufferDesc)
{
    const NRI_NAME(Descriptor)* storageBuffer;
    uint32_t value;
    uint32_t setIndexInPipelineLayout;
    uint32_t rangeIndex;
    uint32_t offsetInRange;
};

NRI_STRUCT(ClearStorageTextureDesc)
{
    const NRI_NAME(Descriptor)* storageTexture;
    NRI_NAME(ClearValue) value;
    uint32_t setIndexInPipelineLayout;
    uint32_t rangeIndex;
    uint32_t offsetInRange;
};

// Descriptor set
NRI_STRUCT(DescriptorRangeUpdateDesc)
{
    const NRI_NAME(Descriptor)* const* descriptors;
    uint32_t descriptorNum;
    uint32_t offsetInRange;
};

NRI_STRUCT(DescriptorSetCopyDesc)
{
    const NRI_NAME(DescriptorSet)* srcDescriptorSet;
    uint32_t baseSrcRange;
    uint32_t baseDstRange;
    uint32_t rangeNum;
    uint32_t baseSrcDynamicConstantBuffer;
    uint32_t baseDstDynamicConstantBuffer;
    uint32_t dynamicConstantBufferNum;
};

// Command signatures (default)
// To fill commands for indirect drawing in a shader use one of "NRI_FILL_X_DESC" macros
NRI_STRUCT(DrawDesc) // see NRI_FILL_DRAW_COMMAND
{
    uint32_t vertexNum;
    uint32_t instanceNum;
    uint32_t baseVertex; // vertex buffer offset = CmdSetVertexBuffers.offset + baseVertex * VertexStreamDesc::stride
    uint32_t baseInstance;
};

NRI_STRUCT(DrawIndexedDesc) // see NRI_FILL_DRAW_INDEXED_COMMAND
{
    uint32_t indexNum;
    uint32_t instanceNum;
    uint32_t baseIndex; // index buffer offset = CmdSetIndexBuffer.offset + baseIndex * sizeof(CmdSetIndexBuffer.indexType)
    int32_t baseVertex; // index += baseVertex
    uint32_t baseInstance;
};

NRI_STRUCT(DispatchDesc)
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

// Modified draw command signatures (D3D12 only)
//  If "DeviceDesc::isDrawParametersEmulationEnabled = true" (emulation globally enabled and allowed) and if a shader has "PipelineLayout::enableDrawParametersEmulation = true" (emulation requested)
//  - the following structs must be used instead
// - "NRI_ENABLE_DRAW_PARAMETERS_EMULATION" must be defined prior inclusion of "NRICompatibility.hlsli"
NRI_STRUCT(DrawBaseDesc) // see NRI_FILL_DRAW_COMMAND
{
    uint32_t shaderEmulatedBaseVertex; // root constant
    uint32_t shaderEmulatedBaseInstance; // root constant
    uint32_t vertexNum;
    uint32_t instanceNum;
    uint32_t baseVertex; // vertex buffer offset = CmdSetVertexBuffers.offset + baseVertex * VertexStreamDesc::stride
    uint32_t baseInstance;
};

NRI_STRUCT(DrawIndexedBaseDesc) // see NRI_FILL_DRAW_INDEXED_COMMAND
{
    int32_t shaderEmulatedBaseVertex; // root constant
    uint32_t shaderEmulatedBaseInstance; // root constant
    uint32_t indexNum;
    uint32_t instanceNum;
    uint32_t baseIndex; // index buffer offset = CmdSetIndexBuffer.offset + baseIndex * sizeof(CmdSetIndexBuffer.indexType)
    int32_t baseVertex; // index += baseVertex
    uint32_t baseInstance;
};

#pragma endregion

//===============================================================================================================================
// Queries
//===============================================================================================================================
#pragma region [ Queries ]

NRI_ENUM
(
    QueryType, uint8_t,

    TIMESTAMP,
    TIMESTAMP_COPY_QUEUE, // requires "isCopyQueueTimestampSupported"
    OCCLUSION,
    PIPELINE_STATISTICS,
    ACCELERATION_STRUCTURE_COMPACTED_SIZE,

    MAX_NUM
);

NRI_STRUCT(QueryPoolDesc)
{
    NRI_NAME(QueryType) queryType;
    uint32_t capacity;
};

// Data layout for QueryType::PIPELINE_STATISTICS
// Never used, only describes the data layout for various cases
NRI_STRUCT(PipelineStatisticsDesc)
{
    // Common part
    uint64_t inputVertexNum;
    uint64_t inputPrimitiveNum;
    uint64_t vertexShaderInvocationNum;
    uint64_t geometryShaderInvocationNum;
    uint64_t geometryShaderPrimitiveNum;
    uint64_t rasterizerInPrimitiveNum;
    uint64_t rasterizerOutPrimitiveNum;
    uint64_t fragmentShaderInvocationNum;
    uint64_t tessControlShaderInvocationNum;
    uint64_t tessEvaluationShaderInvocationNum;
    uint64_t computeShaderInvocationNum;

    // If "isMeshShaderPipelineStatsSupported"
    uint64_t meshControlShaderInvocationNum;
    uint64_t meshEvaluationShaderInvocationNum;

    // If "isMeshShaderPipelineStatsSupported" and D3D12
    uint64_t meshEvaluationShaderPrimitiveNum;
};

#pragma endregion

//===============================================================================================================================
// Device desc
//===============================================================================================================================
#pragma region [ Device desc ]

NRI_STRUCT(AdapterDesc)
{
    char description[128];
    uint64_t luid;
    uint64_t videoMemorySize;
    uint64_t systemMemorySize;
    uint32_t deviceId;
    NRI_NAME(Vendor) vendor;
};

NRI_STRUCT(DeviceDesc)
{
    // Common
    NRI_NAME(AdapterDesc) adapterDesc;
    NRI_NAME(GraphicsAPI) graphicsAPI;
    uint16_t nriVersionMajor;
    uint16_t nriVersionMinor;

    // Viewports
    uint32_t viewportMaxNum;
    uint32_t viewportSubPixelBits;
    int32_t viewportBoundsRange[2];

    // Attachments
    NRI_NAME(Dim_t) attachmentMaxDim;
    NRI_NAME(Dim_t) attachmentLayerMaxNum;
    NRI_NAME(Dim_t) colorAttachmentMaxNum;

    // Multi-sampling
    NRI_NAME(Sample_t) colorSampleMaxNum;
    NRI_NAME(Sample_t) depthSampleMaxNum;
    NRI_NAME(Sample_t) stencilSampleMaxNum;
    NRI_NAME(Sample_t) zeroAttachmentsSampleMaxNum;
    NRI_NAME(Sample_t) textureColorSampleMaxNum;
    NRI_NAME(Sample_t) textureIntegerSampleMaxNum;
    NRI_NAME(Sample_t) textureDepthSampleMaxNum;
    NRI_NAME(Sample_t) textureStencilSampleMaxNum;
    NRI_NAME(Sample_t) storageTextureSampleMaxNum;

    // Resource dimensions
    NRI_NAME(Dim_t) texture1DMaxDim;
    NRI_NAME(Dim_t) texture2DMaxDim;
    NRI_NAME(Dim_t) texture3DMaxDim;
    NRI_NAME(Dim_t) textureArrayMaxDim;
    uint32_t texelBufferMaxDim;

    // Memory
    uint64_t deviceUploadHeapSize; // ReBAR
    uint32_t memoryAllocationMaxNum;
    uint32_t samplerAllocationMaxNum;
    uint32_t uploadBufferTextureRowAlignment;
    uint32_t uploadBufferTextureSliceAlignment;
    uint32_t typedBufferOffsetAlignment;
    uint32_t constantBufferOffsetAlignment;
    uint32_t constantBufferMaxRange;
    uint32_t storageBufferOffsetAlignment;
    uint32_t storageBufferMaxRange;
    uint32_t bufferTextureGranularity;
    uint64_t bufferMaxSize;
    uint32_t pushConstantsMaxSize;

    // Shader resources
    uint32_t boundDescriptorSetMaxNum;
    uint32_t perStageDescriptorSamplerMaxNum;
    uint32_t perStageDescriptorConstantBufferMaxNum;
    uint32_t perStageDescriptorStorageBufferMaxNum;
    uint32_t perStageDescriptorTextureMaxNum;
    uint32_t perStageDescriptorStorageTextureMaxNum;
    uint32_t perStageResourceMaxNum;

    // Descriptor set
    uint32_t descriptorSetSamplerMaxNum;
    uint32_t descriptorSetConstantBufferMaxNum;
    uint32_t descriptorSetStorageBufferMaxNum;
    uint32_t descriptorSetTextureMaxNum;
    uint32_t descriptorSetStorageTextureMaxNum;

    // Vertex shader
    uint32_t vertexShaderAttributeMaxNum;
    uint32_t vertexShaderStreamMaxNum;
    uint32_t vertexShaderOutputComponentMaxNum;

    // Tessellation shaders
    float tessControlShaderGenerationMaxLevel;
    uint32_t tessControlShaderPatchPointMaxNum;
    uint32_t tessControlShaderPerVertexInputComponentMaxNum;
    uint32_t tessControlShaderPerVertexOutputComponentMaxNum;
    uint32_t tessControlShaderPerPatchOutputComponentMaxNum;
    uint32_t tessControlShaderTotalOutputComponentMaxNum;
    uint32_t tessEvaluationShaderInputComponentMaxNum;
    uint32_t tessEvaluationShaderOutputComponentMaxNum;

    // Geometry shader
    uint32_t geometryShaderInvocationMaxNum;
    uint32_t geometryShaderInputComponentMaxNum;
    uint32_t geometryShaderOutputComponentMaxNum;
    uint32_t geometryShaderOutputVertexMaxNum;
    uint32_t geometryShaderTotalOutputComponentMaxNum;

    // Fragment shader
    uint32_t fragmentShaderInputComponentMaxNum;
    uint32_t fragmentShaderOutputAttachmentMaxNum;
    uint32_t fragmentShaderDualSourceAttachmentMaxNum;
    uint32_t fragmentShaderCombinedOutputResourceMaxNum;

    // Compute shader
    uint32_t computeShaderSharedMemoryMaxSize;
    uint32_t computeShaderWorkGroupMaxNum[3];
    uint32_t computeShaderWorkGroupInvocationMaxNum;
    uint32_t computeShaderWorkGroupMaxDim[3];

    // Ray tracing
    uint64_t rayTracingShaderGroupIdentifierSize;
    uint64_t rayTracingShaderTableAligment;
    uint64_t rayTracingShaderTableMaxStride;
    uint32_t rayTracingShaderRecursionMaxDepth;
    uint32_t rayTracingGeometryObjectMaxNum;

    // Mesh shaders
    uint32_t meshControlSharedMemoryMaxSize;
    uint32_t meshControlWorkGroupInvocationMaxNum;
    uint32_t meshControlPayloadMaxSize;
    uint32_t meshEvaluationOutputVerticesMaxNum;
    uint32_t meshEvaluationOutputPrimitiveMaxNum;
    uint32_t meshEvaluationOutputComponentMaxNum;
    uint32_t meshEvaluationSharedMemoryMaxSize;
    uint32_t meshEvaluationWorkGroupInvocationMaxNum;

    // Other
    uint64_t timestampFrequencyHz;
    uint32_t subPixelPrecisionBits;
    uint32_t subTexelPrecisionBits;
    uint32_t mipmapPrecisionBits;
    uint32_t drawIndexedIndex16ValueMax;
    uint32_t drawIndexedIndex32ValueMax;
    uint32_t drawIndirectMaxNum;
    float samplerLodBiasMin;
    float samplerLodBiasMax;
    float samplerAnisotropyMax;
    int32_t texelOffsetMin;
    uint32_t texelOffsetMax;
    int32_t texelGatherOffsetMin;
    uint32_t texelGatherOffsetMax;
    uint32_t clipDistanceMaxNum;
    uint32_t cullDistanceMaxNum;
    uint32_t combinedClipAndCullDistanceMaxNum;
    uint8_t conservativeRasterTier;
    uint8_t programmableSampleLocationsTier;

    // Features
    uint32_t isComputeQueueSupported : 1;
    uint32_t isCopyQueueSupported : 1;
    uint32_t isTextureFilterMinMaxSupported : 1;
    uint32_t isLogicOpSupported : 1;
    uint32_t isDepthBoundsTestSupported : 1;
    uint32_t isDrawIndirectCountSupported : 1;
    uint32_t isIndependentFrontAndBackStencilReferenceAndMasksSupported : 1;
    uint32_t isLineSmoothingSupported : 1;
    uint32_t isCopyQueueTimestampSupported : 1;
    uint32_t isDispatchRaysIndirectSupported : 1;
    uint32_t isDrawMeshTasksIndirectSupported : 1;
    uint32_t isMeshShaderPipelineStatsSupported : 1;

    // Shader features
    uint32_t isShaderNativeI16Supported : 1;
    uint32_t isShaderNativeF16Supported : 1;
    uint32_t isShaderNativeI32Supported : 1;
    uint32_t isShaderNativeF32Supported : 1;
    uint32_t isShaderNativeI64Supported : 1;
    uint32_t isShaderNativeF64Supported : 1;

    uint32_t isShaderAtomicsI16Supported : 1;
    uint32_t isShaderAtomicsF16Supported : 1;
    uint32_t isShaderAtomicsI32Supported : 1;
    uint32_t isShaderAtomicsF32Supported : 1;
    uint32_t isShaderAtomicsI64Supported : 1;
    uint32_t isShaderAtomicsF64Supported : 1;

    // Emulated features
    uint32_t isDrawParametersEmulationEnabled : 1;

    // Extensions (unexposed are always supported)
    uint32_t isSwapChainSupported : 1; // NRISwapChain
    uint32_t isRayTracingSupported : 1; // NRIRayTracing
    uint32_t isMeshShaderSupported : 1; // NRIMeshShader
    uint32_t isLowLatencySupported : 1; // NRILowLatency
};

#pragma endregion

NRI_NAMESPACE_END
