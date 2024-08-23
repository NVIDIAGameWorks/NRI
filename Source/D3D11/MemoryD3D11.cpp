// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"

#include "MemoryD3D11.h"

using namespace nri;

Result MemoryD3D11::Create(const AllocateMemoryDesc& allocateMemoryDesc) {
    m_Location = (MemoryLocation)allocateMemoryDesc.type;
    m_Priority = allocateMemoryDesc.priority;

    return Result::SUCCESS;
}
