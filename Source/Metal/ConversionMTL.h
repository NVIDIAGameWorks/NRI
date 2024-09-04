#pragma once

#include <array>

namespace nri {

constexpr std::array<MTLBlendOperation, (size_t)BlendFunc::MAX_NUM> BLEND_OP = {
    MTLBlendOperationAdd,              // ADD
    MTLBlendOperationSubtract,         // SUBTRACT
    MTLBlendOperationReverseSubtract,  // REVERSE_SUBTRACT
    MTLBlendOperationMin,              // MIN
    MTLBlendOperationMax               // MAX
};
constexpr MTLBlendOperation GetBlendOp(BlendFunc blendFunc) {
    return BLEND_OP[(size_t)blendFunc];
}

constexpr std::array<MTLTextureType, (size_t)TextureType::MAX_NUM> IMAGE_TYPES = {
    MTLTextureType1D, // TEXTURE_1D
    MTLTextureType2D, // TEXTURE_2D
    MTLTextureType3D, // TEXTURE_3D
};

constexpr MTLTextureType GetImageTypeMTL(TextureType type) {
    return IMAGE_TYPES[(size_t)type];
}

constexpr std::array<MTLBlendFactor, (size_t)BlendFactor::MAX_NUM> BLEND_FACTOR = {
    MTLBlendFactorZero,                     // ZERO
    MTLBlendFactorOne,                      // ONE
    MTLBlendFactorSourceColor,              // SRC_COLOR
    MTLBlendFactorOneMinusSourceColor,      // ONE_MINUS_SRC_COLOR
    MTLBlendFactorDestinationColor,         // DST_COLOR
    MTLBlendFactorOneMinusDestinationColor, // ONE_MINUS_DST_COLOR
    MTLBlendFactorSourceAlpha,              // SRC_ALPHA
    MTLBlendFactorOneMinusSourceAlpha,      // ONE_MINUS_SRC_ALPHA
    MTLBlendFactorDestinationAlpha,         // DST_ALPHA
    MTLBlendFactorOneMinusDestinationAlpha, // ONE_MINUS_DST_ALPHA
    MTLBlendFactorBlendColor,               // CONSTANT_COLOR
    MTLBlendFactorOneMinusBlendColor,       // ONE_MINUS_CONSTANT_COLOR
    MTLBlendFactorBlendAlpha,               // CONSTANT_ALPHA
    MTLBlendFactorOneMinusBlendAlpha,       // ONE_MINUS_CONSTANT_ALPHA
    MTLBlendFactorSourceAlphaSaturated,     // SRC_ALPHA_SATURATE
    MTLBlendFactorSource1Color,             // SRC1_COLOR
    MTLBlendFactorOneMinusSource1Color,     // ONE_MINUS_SRC1_COLOR
    MTLBlendFactorSource1Alpha,             // SRC1_ALPHA
    MTLBlendFactorOneMinusSource1Alpha,     // ONE_MINUS_SRC1_ALPHA
};

constexpr MTLBlendFactor GetBlendFactor(BlendFactor blendFactor) {
    return BLEND_FACTOR[(size_t)blendFactor];
}

constexpr MTLColorWriteMask GetColorComponent(ColorWriteBits colorWriteMask) {
    return MTLColorWriteMask(colorWriteMask) & MTLColorWriteMaskAll;
}

constexpr std::array<MTLDataType, (size_t)DescriptorType::MAX_NUM> DESCRIPTOR_TYPES = {
    MTLDataTypeSampler, // SAMPLER
    MTLDataTypeNone,    // CONSTANT_BUFFER
    MTLDataTypeTexture, // TEXTURE
    MTLDataTypeNone,    // STORAGE_TEXTURE
    MTLDataTypeStruct,  // BUFFER
    MTLDataTypeStruct,  // STORAGE_BUFFER
    MTLDataTypeArray,   // STRUCTURED_BUFFER
    MTLDataTypeStruct,    // STORAGE_STRUCTURED_BUFFER
    MTLDataTypePrimitiveAccelerationStructure     // ACCELERATION_STRUCTURE
};

constexpr MTLDataType GetDescriptorType(DescriptorType type) {
    return DESCRIPTOR_TYPES[(size_t)type];
}

constexpr std::array<MTLCompareFunction, (size_t)CompareFunc::MAX_NUM> COMPARE_OP = {
    MTLCompareFunctionNever,            // NONE
    MTLCompareFunctionAlways,           // ALWAYS
    MTLCompareFunctionNever,            // NEVER
    MTLCompareFunctionEqual,            // EQUAL
    MTLCompareFunctionNotEqual,         // NOT_EQUAL
    MTLCompareFunctionLess,             // LESS
    MTLCompareFunctionLessEqual,        // LESS_EQUAL
    MTLCompareFunctionGreater,          // GREATER
    MTLCompareFunctionGreaterEqual,     // GREATER_EQUAL
};

constexpr MTLCompareFunction GetCompareOp(CompareFunc compareFunc) {
    return COMPARE_OP[(size_t)compareFunc];
}

constexpr std::array<MTLStencilOperation, (size_t)StencilFunc::MAX_NUM> STENCIL_OP = {
    MTLStencilOperationKeep,                // KEEP
    MTLStencilOperationZero,                // ZERO
    MTLStencilOperationReplace,             // REPLACE
    MTLStencilOperationIncrementClamp,      // INCREMENT_AND_CLAMP
    MTLStencilOperationDecrementClamp,      // DECREMENT_AND_CLAMP
    MTLStencilOperationInvert,              // INVERT
    MTLStencilOperationIncrementWrap,       // INCREMENT_AND_WRAP
    MTLStencilOperationDecrementWrap        // DECREMENT_AND_WRAP
};

constexpr MTLStencilOperation GetStencilOp(StencilFunc stencilFunc) {
    return STENCIL_OP[(size_t)stencilFunc];
}


constexpr std::array<MTLCullMode, (size_t)CullMode::MAX_NUM> CULL_MODES = {
    MTLCullModeNone,  // NONE
    MTLCullModeFront, // FRONT
    MTLCullModeBack   // BACK
};

constexpr MTLCullMode GetCullMode(CullMode cullMode) {
    return CULL_MODES[(size_t)cullMode];
}

constexpr std::array<MTLTriangleFillMode, (size_t)FillMode::MAX_NUM> POLYGON_MODES = {
    MTLTriangleFillModeFill, // SOLID
    MTLTriangleFillModeLines // WIREFRAME
};

constexpr MTLTriangleFillMode GetPolygonMode(FillMode fillMode) {
    return POLYGON_MODES[(size_t)fillMode];
}

constexpr std::array<MTLSamplerAddressMode, (size_t)AddressMode::MAX_NUM> SAMPLER_ADDRESS_MODE = {
    MTLSamplerAddressModeRepeat,          // REPEAT
    MTLSamplerAddressModeMirrorRepeat,    // MIRRORED_REPEAT
    MTLSamplerAddressModeClampToEdge,     // CLAMP_TO_EDGE
    MTLSamplerAddressModeClampToZero      // CLAMP_TO_BORDER
};

constexpr MTLSamplerAddressMode GetSamplerAddressMode(AddressMode addressMode) {
    return SAMPLER_ADDRESS_MODE[(size_t)addressMode];
}



constexpr std::array<MTLPrimitiveTopologyClass, (size_t)Topology::MAX_NUM> TOPOLOGIES = {
    MTLPrimitiveTopologyClassPoint,           // POINT_LIST
    MTLPrimitiveTopologyClassLine,            // LINE_LIST
    MTLPrimitiveTopologyClassLine,            // LINE_STRIP
    MTLPrimitiveTopologyClassTriangle,        // TRIANGLE_LIST
    MTLPrimitiveTopologyClassTriangle,        // TRIANGLE_STRIP
    MTLPrimitiveTopologyClassLine,            // LINE_LIST_WITH_ADJACENCY
    MTLPrimitiveTopologyClassLine,            // LINE_STRIP_WITH_ADJACENCY
    MTLPrimitiveTopologyClassTriangle,        // TRIANGLE_LIST_WITH_ADJACENCY
    MTLPrimitiveTopologyClassTriangle,        // TRIANGLE_STRIP_WITH_ADJACENCY
    MTLPrimitiveTopologyClassTriangle         // PATCH_LIST TODO: not sure
};


constexpr MTLPrimitiveTopologyClass GetTopologyMTL(Topology topology) {
    return TOPOLOGIES[(size_t)topology];
}

inline MTLPixelFormat GetFormatMTL(Format format, bool demoteSrgb = false) {
    if (demoteSrgb) {
        const FormatProps& formatProps = GetFormatProps(format);
        if (formatProps.isSrgb)
            format = (Format)((uint32_t)format - 1);
    }
    
    return (MTLPixelFormat)NRIFormatToMTLFormat(format);
}

};

