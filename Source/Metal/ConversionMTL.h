#pragma once

#import <MetalKit/MetalKit.h>

#include "NRIMacro.h"
#include "NRIDescs.h"
#include "Extensions/NRIWrapperVK.h"

namespace nri {

    constexpr std::array<MTLTextureType, (size_t)TextureType::MAX_NUM> IMAGE_TYPES = {
        MTLTextureType1D, // TEXTURE_1D
        MTLTextureType2D, // TEXTURE_2D
        MTLTextureType3D, // TEXTURE_3D   
    };

    constexpr MTLTextureType GetImageTypeMTL(TextureType type) {
        return IMAGE_TYPES[(size_t)type];
    }


    inline MTLTextureType GetFormatMTL(Format format, bool demoteSrgb = false) {
        if (demoteSrgb) {
            const FormatProps& formatProps = GetFormatProps(format);
            if (formatProps.isSrgb)
                format = (Format)((uint32_t)format - 1);
        }

        return (MTLTextureType)NRIFormatToMTLFormat(format);
    }

};

