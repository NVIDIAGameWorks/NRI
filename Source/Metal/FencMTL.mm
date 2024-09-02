#include "SharedMTL.h"

#include "FenceMTL.h"

using namespace nri;

FenceMTL::~FenceMTL() {

}

Result FenceMTL::Create(uint64_t initialValue) {
    m_Handle = dispatch_semaphore_create(initialValue);
}
