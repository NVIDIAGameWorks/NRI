#include "ConversionMTL.h"

// Each depth/stencil format is only compatible with itself in VK
constexpr std::array<MTLTextureType,(uint32_t)Format::MAX_NUM> MLT_FORMAT = {
    MTLPixelFormatInvalid,             // UNKNOWN
    MTLPixelFormatR8Unorm,             // R8_UNORM
    MTLPixelFormatR8Snorm,             // R8_SNORM
    MTLPixelFormatR8Uint,              // R8_UINT
    MTLPixelFormatR8Sint,              // R8_SINT
    MTLPixelFormatRG8Unorm,            // RG8_UNORM
    MTLPixelFormatRG8Snorm,            // RG8_SNORM
    MTLPixelFormatRG8Uint,             // RG8_UINT
    MTLPixelFormatRG8Sint,             // RG8_SINT
    MTLPixelFormatBGRA8Unorm,          // BGRA8_UNORM
    MTLPixelFormatBGRA8Unorm_sRGB,     // BGRA8_SRGB
    MTLPixelFormatRGBA8Unorm,          // RGBA8_UNORM
    MTLPixelFormatRGBA8Unorm_sRGB,     // RGBA8_SRGB
    MTLPixelFormatRGBA8Snorm,          // RGBA8_SNORM
    MTLPixelFormatRGBA8Uint,           // RGBA8_UINT
    MTLPixelFormatRGBA8Sint,           // RGBA8_SINT
    MTLPixelFormatR16Unorm,            // R16_UNORM
    MTLPixelFormatR16Snorm,            // R16_SNORM
    MTLPixelFormatR16Uint,             // R16_UINT
    MTLPixelFormatR16Sint,             // R16_SINT
    MTLPixelFormatR16Float,            // R16_SFLOAT
    MTLPixelFormatRG16Unorm,           // RG16_UNORM
    MTLPixelFormatRG16Snorm,           // RG16_SNORM
    MTLPixelFormatRG16Uint,            // RG16_UINT
    MTLPixelFormatRG16Sint,            // RG16_SINT
    MTLPixelFormatRG16Float,           // RG16_SFLOAT
    MTLPixelFormatRGBA16Unorm,         // RGBA16_UNORM
    MTLPixelFormatRGBA16Snorm,         // RGBA16_SNORM
    MTLPixelFormatRGBA16Uint,          // RGBA16_UINT
    MTLPixelFormatRGBA16Sint,          // RGBA16_SINT
    MTLPixelFormatRGBA16Float,         // RGBA16_SFLOAT
    MTLPixelFormatR32Uint,             // R32_UINT
    MTLPixelFormatR32Sint,             // R32_SINT
    MTLPixelFormatR32Float,            // R32_SFLOAT
    MTLPixelFormatRG32Uint,            // RG32_UINT
    MTLPixelFormatRG32Sint,            // RG32_SINT
    MTLPixelFormatRG32Float,           // RG32_SFLOAT
    MTLPixelFormatRGB32Uint,           // RGB32_UINT
    MTLPixelFormatRGB32Sint,           // RGB32_SINT
    MTLPixelFormatRGB32Float,          // RGB32_SFLOAT
    MTLPixelFormatRGBA32Uint,          // RGB32_UINT
    MTLPixelFormatRGBA32Sint,          // RGB32_SINT
    MTLPixelFormatRGBA32Float,         // RGB32_SFLOAT
    MTLPixelFormatB5G6R5Unorm,         // B5_G6_R5_UNORM
    MTLPixelFormatA1BGR5Unorm,         // B5_G5_R5_A1_UNORM
    MTLPixelFormatABGR4Unorm,          // B4_G4_R4_A4_UNORM
    MTLPixelFormatRGB10A2Unorm,        // R10_G10_B10_A2_UNORM
    MTLPixelFormatRGB10A2Uint,         // R10_G10_B10_A2_UINT
    MTLPixelFormatRG11B10Float,        // R11_G11_B10_UFLOAT
    MTLPixelFormatRGB9E5Float,         // R9_G9_B9_E5_UFLOAT
    MTLPixelFormatBC1_RGBA,            // BC1_RGBA_UNORM
    MTLPixelFormatBC1_RGBA_sRGB,       // BC1_RGBA_SRGB
    MTLPixelFormatBC2_RGBA,            // BC2_RGBA_UNORM
    MTLPixelFormatBC2_RGBA_sRGB,       // BC2_RGBA_SRGB
    MTLPixelFormatBC3_RGBA,            // BC3_RGBA_UNORM
    MTLPixelFormatBC3_RGBA_sRGB,       // BC3_RGBA_SRGB
    MTLPixelFormatBC4_RUnorm,          // BC4_R_UNORM
    MTLPixelFormatBC4_RSnorm,          // BC4_R_SNORM
    MTLPixelFormatBC5_RGUnorm,         // BC5_RG_UNORM
    MTLPixelFormatBC5_RGSnorm,         // BC5_RG_SNORM
    MTLPixelFormatBC6H_RGBUfloat,      // BC6H_RGB_UFLOAT
    MTLPixelFormatBC6H_RGBFloat,       // BC6H_RGB_SFLOAT
    MTLPixelFormatBC7_RGBAUnorm,       // BC7_RGBA_UNORM
    MTLPixelFormatBC7_RGBAUnorm_sRGB,  // BC7_RGBA_SRGB
    MTLPixelFormatDepth16Unorm,        // D16_UNORM
    MTLPixelFormatDepth24Unorm_Stencil8, // D24_UNORM_S8_UINT
    MTLPixelFormatDepth32Float,        // D32_SFLOAT
    MTLPixelFormatDepth32Float_Stencil8, // D32_SFLOAT_S8_UINT_X24
    MTLPixelFormatDepth24Unorm_Stencil8, // R24_UNORM_X8
    MTLPixelFormatDepth24Unorm_Stencil8, // X24_G8_UINT
    MTLPixelFormatDepth32Float_Stencil8, // R32_SFLOAT_X8_X24
    MTLPixelFormatDepth32Float_Stencil8, // X32_G8_UINT_X24
};

uint32_t NRIFormatToMTLFormat(Format format) {
    return (uint32_t)VK_FORMAT[(uint32_t)format];
}

