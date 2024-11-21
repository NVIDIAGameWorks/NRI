#include "SharedMTL.h"

#include "BufferMTL.h"

using namespace nri;


Result BufferMTL::Create(const BufferDesc& bufferDesc) {
    return Result::SUCCESS;
}

Result BufferMTL::Create(const BufferVKDesc& bufferDesc) {
    return Result::SUCCESS;
}


void* BufferMTL::Map(uint64_t offset, uint64_t size) {
    return (uint8_t*)[pBuffer contents] + offset;
}

void BufferMTL::Unmap() {
    
}


Result BufferMTL::Create(const AllocateBufferDesc& bufferDesc) {
    return Result::SUCCESS;
}

void BufferMTL::SetDebugName(const char* name) {
    
    NSString* str = [NSString stringWithUTF8String:name];


    //[pBuffer addDebugMarker:name range:]
    
}
