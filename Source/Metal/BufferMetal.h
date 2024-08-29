

#import <MetalKit/MetalKit.h>

struct BufferMetal {

private:
    id<MTLBuffer>                pBuffer;
    //id<MTLIndirectCommandBuffer> pIndirectCommandBuffer;
    uint64_t                     mOffset;
};



