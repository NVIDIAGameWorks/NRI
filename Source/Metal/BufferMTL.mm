#include "SharedMTL.h"

#include "BufferMTL.h"
#include "MemoryMTL.h"

using namespace nri;

void* BufferMTL::Map(uint64_t offset, uint64_t size) {
    return (uint8_t*)[m_Handle contents] + offset;
}

BufferMTL::~BufferMTL() {
    [m_Label release];
}

void BufferMTL::Unmap() {
    
}

void BufferMTL::FinishMemoryBinding(MemoryMTL& memory, uint64_t memoryOffset) {
    m_Handle =  [memory.GetHandle()
                 newBufferWithLength: m_Desc.size
                 options: MTLResourceCPUCacheModeDefaultCache
                 offset: memoryOffset];
    UpdateLabel();
}

Result BufferMTL::Create(const AllocateBufferDesc& bufferDesc) {
    m_Desc = bufferDesc.desc;
    m_Handle = [m_Device.GetHandle() newBufferWithLength: m_Desc.size options: MTLResourceCPUCacheModeDefaultCache];
    UpdateLabel();
}

void BufferMTL::UpdateLabel() {
    if(m_Handle && m_Label) {
        [m_Handle setLabel: m_Label];
    }
}

Result BufferMTL::Create(const BufferDesc& bufferDesc) {
    m_Desc = bufferDesc;
}

void BufferMTL::SetDebugName(const char* name) {
    m_Label  = [NSString stringWithUTF8String:name];
    [m_Label retain];
    UpdateLabel();
}
