// © 2024 NVIDIA Corporation

#ifndef NRI_COMPATIBILITY_HLSLI
#define NRI_COMPATIBILITY_HLSLI

/*
USAGE:

Textures, buffers, samplers and acceleration structures:
    NRI_RESOURCE(Texture2D<float4>, gInput, t, 0, 2);
    NRI_RESOURCE(RWTexture2D<float>, gOutputs, u, 0, 0);
    NRI_RESOURCE(StructuredBuffer<InstanceData>, gInstanceData, t, 2, 2);
    NRI_RESOURCE(RaytracingAccelerationStructure, gTlas, t, 1, 2);
    NRI_RESOURCE(SamplerState, gLinearMipmapLinearSampler, s, 0, 0);

Texture and buffer arrays:
    NRI_RESOURCE(Texture2D<float3>, gInputs[], t, 0, 1); // DXIL/SPIRV only
    NRI_RESOURCE(Texture2D<float>, gInputs[8], t, 0, 0); // DXBC compatible

Constants:
    NRI_RESOURCE(cbuffer, Constants, b, 0, 3) {
        uint32_t gConst1;
        uint32_t gConst2;
        uint32_t gConst3;
        uint32_t gConst4;
    };

Push constants:
    struct PushConstants {
        float const1;
        uint32_t const2;
    };

    NRI_PUSH_CONSTANTS(PushConstants, gPushConstants, 7); // a constant buffer in DXBC

Draw parameters:
    - Add to the global scope:
        NRI_ENABLE_DRAW_PARAMETERS;
    - Add to a function input parameters list:
        void main(..., NRI_DECLARE_DRAW_PARAMETERS) {
            ...
        }
    - Use the following macros:
        NRI_VERTEX_ID, NRI_INSTANCE_ID - start from 0
        NRI_BASE_VERTEX, NRI_BASE_INSTANCE - base vertex / instance from a "Draw" call
        NRI_VERTEX_ID_OFFSET, NRI_INSTANCE_ID_OFFSET - start from "base" vertex / instance
    - To fill commands for indirect drawing in a shader use one of "NRI_FILL_X_DESC" macros
    - "NRI_ENABLE_DRAW_PARAMETERS_EMULATION" must be defined prior inclusion of "NRICompatibility.hlsli"
      for pipelines expecting emulation
*/

#ifndef __cplusplus
    #define NRI_MERGE_TOKENS(a, b) a##b
#endif

// Container detection
#ifdef __hlsl_dx_compiler
    #ifdef __spirv__
        #define NRI_SPIRV
        #define NRI_PRINTF_AVAILABLE
    #else
        #define NRI_DXIL
    #endif
#else
    #ifndef __cplusplus
        #define NRI_DXBC
    #endif
#endif

// Shader model
#ifdef NRI_DXBC
    #define NRI_SHADER_MODEL 50
#else
    #define NRI_SHADER_MODEL (__SHADER_TARGET_MAJOR * 10 + __SHADER_TARGET_MINOR)
#endif

// Indirect commands filling // TODO: change to StructuredBuffers?
#define NRI_DRAW_DESC_SIZE (4 * 4) // "DrawDesc"
#define NRI_FILL_DRAW_DESC(buffer, cmdIndex, vertexNum, instanceNum, baseVertex, baseInstance) \
    buffer.Store(cmdIndex * NRI_DRAW_DESC_SIZE + 0, vertexNum); \
    buffer.Store(cmdIndex * NRI_DRAW_DESC_SIZE + 4, instanceNum); \
    buffer.Store(cmdIndex * NRI_DRAW_DESC_SIZE + 8, baseVertex); \
    buffer.Store(cmdIndex * NRI_DRAW_DESC_SIZE + 12, baseInstance)

#define NRI_DRAW_INDEXED_DESC_SIZE (5 * 4) // see "DrawIndexedDesc"
#define NRI_FILL_DRAW_INDEXED_DESC(buffer, cmdIndex, indexNum, instanceNum, baseIndex, baseVertex, baseInstance) \
    buffer.Store(cmdIndex * NRI_DRAW_INDEXED_DESC_SIZE + 0, indexNum); \
    buffer.Store(cmdIndex * NRI_DRAW_INDEXED_DESC_SIZE + 4, instanceNum); \
    buffer.Store(cmdIndex * NRI_DRAW_INDEXED_DESC_SIZE + 8, baseIndex); \
    buffer.Store(cmdIndex * NRI_DRAW_INDEXED_DESC_SIZE + 12, baseVertex); \
    buffer.Store(cmdIndex * NRI_DRAW_INDEXED_DESC_SIZE + 16, baseInstance)

#define NRI_DRAW_BASE_DESC_SIZE (6 * 4) // see "DrawBaseDesc"
#define NRI_FILL_DRAW_BASE_DESC(buffer, cmdIndex, vertexNum, instanceNum, baseVertex, baseInstance) \
    buffer.Store(cmdIndex * NRI_DRAW_DESC_SIZE + 0, baseVertex); /* root constant */ \
    buffer.Store(cmdIndex * NRI_DRAW_DESC_SIZE + 4, baseInstance); /* root constant */ \
    buffer.Store(cmdIndex * NRI_DRAW_DESC_SIZE + 8, vertexNum); \
    buffer.Store(cmdIndex * NRI_DRAW_DESC_SIZE + 12, instanceNum); \
    buffer.Store(cmdIndex * NRI_DRAW_DESC_SIZE + 16, baseVertex); \
    buffer.Store(cmdIndex * NRI_DRAW_DESC_SIZE + 20, baseInstance)

#define NRI_DRAW_INDEXED_BASE_DESC_SIZE (7 * 4) // see "DrawIndexedBaseDesc"
#define NRI_FILL_DRAW_INDEXED_BASE_DESC(buffer, cmdIndex, indexNum, instanceNum, baseIndex, baseVertex, baseInstance) \
    buffer.Store(cmdIndex * NRI_DRAW_INDEXED_DESC_SIZE + 0, baseVertex); /* root constant */ \
    buffer.Store(cmdIndex * NRI_DRAW_INDEXED_DESC_SIZE + 4, baseInstance); /* root constant */ \
    buffer.Store(cmdIndex * NRI_DRAW_INDEXED_DESC_SIZE + 8, indexNum); \
    buffer.Store(cmdIndex * NRI_DRAW_INDEXED_DESC_SIZE + 12, instanceNum); \
    buffer.Store(cmdIndex * NRI_DRAW_INDEXED_DESC_SIZE + 16, baseIndex); \
    buffer.Store(cmdIndex * NRI_DRAW_INDEXED_DESC_SIZE + 20, baseVertex); \
    buffer.Store(cmdIndex * NRI_DRAW_INDEXED_DESC_SIZE + 24, baseInstance)

// SPIRV
#ifdef NRI_SPIRV
    #define NRI_RESOURCE(resourceType, name, regName, bindingIndex, setIndex) \
        resourceType name : register(NRI_MERGE_TOKENS(regName, bindingIndex), NRI_MERGE_TOKENS(space, setIndex))

    #define NRI_PUSH_CONSTANTS(structName, name, bindingIndex) \
        [[vk::push_constant]] structName name

    // Draw parameters (full support, requires SPV_KHR_shader_draw_parameters)
    #define NRI_ENABLE_DRAW_PARAMETERS

    #define NRI_DECLARE_DRAW_PARAMETERS \
        int NRI_VERTEX_ID_OFFSET : SV_VertexID, \
        uint NRI_INSTANCE_ID_OFFSET : SV_InstanceID, \
        [[vk::builtin("BaseVertex")]] int NRI_BASE_VERTEX : _SV_Nothing, \
        [[vk::builtin("BaseInstance")]] uint NRI_BASE_INSTANCE : _SV_Nothing

    #define NRI_VERTEX_ID (NRI_VERTEX_ID_OFFSET - NRI_BASE_VERTEX)
    #define NRI_INSTANCE_ID (NRI_INSTANCE_ID_OFFSET - NRI_BASE_INSTANCE)
#endif

// DXIL
#ifdef NRI_DXIL
    #define NRI_RESOURCE(resourceType, name, regName, bindingIndex, setIndex) \
        resourceType name : register(NRI_MERGE_TOKENS(regName, bindingIndex), NRI_MERGE_TOKENS(space, setIndex))

    #define NRI_PUSH_CONSTANTS(structName, name, bindingIndex) \
        ConstantBuffer<structName> name : register(NRI_MERGE_TOKENS(b, bindingIndex), space0)

    // Draw parameters
    #if (NRI_SHADER_MODEL < 68)
        #ifdef NRI_ENABLE_DRAW_PARAMETERS_EMULATION
            // Full support (emulation)
            #define NRI_ENABLE_DRAW_PARAMETERS \
                struct _BaseAttributeConstants { \
                    int baseVertex; \
                    uint baseInstance; \
                }; \
                ConstantBuffer<_BaseAttributeConstants> _BaseAttributes : register(b0, space999) // see BASE_ATTRIBUTES_EMULATION_SPACE

            #define NRI_DECLARE_DRAW_PARAMETERS \
                uint NRI_VERTEX_ID : SV_VertexID, \
                uint NRI_INSTANCE_ID : SV_InstanceID

            #define NRI_BASE_VERTEX _BaseAttributes.baseVertex
            #define NRI_BASE_INSTANCE _BaseAttributes.baseInstance
            #define NRI_VERTEX_ID_OFFSET (NRI_BASE_VERTEX + NRI_VERTEX_ID)
            #define NRI_INSTANCE_ID_OFFSET (NRI_BASE_INSTANCE + NRI_INSTANCE_ID)

            #undef NRI_DRAW_DESC_SIZE
            #undef NRI_FILL_DRAW_DESC
            #define NRI_DRAW_DESC_SIZE NRI_DRAW_BASE_DESC_SIZE
            #define NRI_FILL_DRAW_DESC NRI_FILL_DRAW_BASE_DESC

            #undef NRI_DRAW_INDEXED_DESC_SIZE
            #undef NRI_FILL_DRAW_INDEXED_DESC
            #define NRI_DRAW_INDEXED_DESC_SIZE NRI_DRAW_INDEXED_BASE_DESC_SIZE
            #define NRI_FILL_DRAW_INDEXED_DESC NRI_FILL_DRAW_INDEXED_BASE_DESC
        #else
            // Partial support
            #define NRI_ENABLE_DRAW_PARAMETERS

            #define NRI_DECLARE_DRAW_PARAMETERS \
                uint NRI_VERTEX_ID : SV_VertexID, \
                uint NRI_INSTANCE_ID : SV_InstanceID

            #define NRI_BASE_VERTEX NRI_BASE_VERTEX_is_unsupported
            #define NRI_BASE_INSTANCE NRI_BASE_INSTANCE_is_unsupported
            #define NRI_VERTEX_ID_OFFSET NRI_VERTEX_ID_OFFSET_is_unsupported
            #define NRI_INSTANCE_ID_OFFSET NRI_INSTANCE_ID_OFFSET_is_unsupported
        #endif
    #else
        // Full support
        #define NRI_ENABLE_DRAW_PARAMETERS

        #define NRI_DECLARE_DRAW_PARAMETERS \
            uint NRI_VERTEX_ID : SV_VertexID, \
            uint NRI_INSTANCE_ID : SV_InstanceID, \
            int NRI_BASE_VERTEX : SV_StartVertexLocation, \
            uint NRI_BASE_INSTANCE : SV_StartInstanceLocation
    #endif
#endif

// DXBC
#ifdef NRI_DXBC
    #define NRI_RESOURCE(resourceType, name, regName, bindingIndex, setIndex) \
        resourceType name : register(NRI_MERGE_TOKENS(regName, bindingIndex))

    #define NRI_PUSH_CONSTANTS(structName, name, bindingIndex) \
        cbuffer structName##_##name : register(NRI_MERGE_TOKENS(b, bindingIndex)) { \
            structName name; \
        }

    // Draw parameters (partial support)
    #define NRI_ENABLE_DRAW_PARAMETERS

    #define NRI_DECLARE_DRAW_PARAMETERS \
        uint NRI_VERTEX_ID : SV_VertexID, \
        uint NRI_INSTANCE_ID : SV_InstanceID

    #define NRI_BASE_VERTEX NRI_BASE_VERTEX_is_unsupported
    #define NRI_BASE_INSTANCE NRI_BASE_INSTANCE_is_unsupported
    #define NRI_VERTEX_ID_OFFSET NRI_VERTEX_ID_OFFSET_is_unsupported
    #define NRI_INSTANCE_ID_OFFSET NRI_INSTANCE_ID_OFFSET_is_unsupported

    // Missing data types
    #define uint32_t uint
    #define uint32_t2 uint2
    #define uint32_t3 uint3
    #define uint32_t4 uint4

    #define int32_t int
    #define int32_t2 int2
    #define int32_t3 int3
    #define int32_t4 int4

    #define float16_t float
    #define float16_t2 float2
    #define float16_t3 float3
    #define float16_t4 float4
#endif

// C/C++
#ifdef __cplusplus
    #define NRI_RESOURCE(resourceType, name, regName, bindingIndex, setIndex) \
        struct name
#endif

// Misc
#define NRI_UV_TO_CLIP(uv) (uv * float2(2, -2) + float2(-1, 1))
#define NRI_CLIP_TO_UV(clip) (clip * float2(0.5, -0.5) + 0.5)

#endif