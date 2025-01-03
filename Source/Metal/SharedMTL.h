#import <MetalKit/MetalKit.h>

#include "SharedExternal.h"

#include "ConversionMTL.h"
#include "DeviceMTL.h"

#pragma once

NriEnum(PipelineType, uint8_t,
    Compute,
    Graphics,
    Raytracing
);

NriBits(BarrierBits, uint8_t,
    NONE = 0,
    BARRIER_FLAG_BUFFERS = NriBit(0),
    BARRIER_FLAG_TEXTURES = NriBit(1),
    BARRIER_FLAG_RENDERTARGETS = NriBit(2),
    BARRIER_FLAG_FENCE = NriBit(3));


NriBits(CommandBufferDirtyBits, uint8_t,
        NONE                            = 0,
        CMD_DIRTY_STENCIL          = NriBit(0),
        CMD_DIRTY_SHADING_RATE = NriBit(1),
        CMD_DIRTY_BLEND_CONSTANT = NriBit(2)
    );


struct MemoryTypeInfo {
    union {
        uint32_t value;
        struct {
            uint32_t options: 16; // MTLResourceOptions
            uint32_t storageMode: 4; // MTLStorageMode
            uint32_t cacheMode: 2; // MTLCPUCacheMode
        };
    };
};
