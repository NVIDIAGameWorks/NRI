#import <MetalKit/MetalKit.h>

#include "SharedExternal.h"

#include "ConversionMTL.h"
#include "DeviceMTL.h"

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
