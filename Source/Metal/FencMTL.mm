#include "SharedMTL.h"

#include "FenceMTL.h"

using namespace nri;

FenceMTL::~FenceMTL() {

}

Result FenceMTL::Create(uint64_t initialValue) {
    m_Handle = dispatch_semaphore_create(initialValue);
}

void FenceMTL::SetDebugName(const char* name) {
    
}

void FenceMTL::Wait(uint64_t value) {
    dispatch_semaphore_wait(m_Handle, DISPATCH_TIME_FOREVER);
}
