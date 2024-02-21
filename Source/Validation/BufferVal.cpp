// © 2021 NVIDIA Corporation

#include "SharedExternal.h"
#include "SharedVal.h"

#include "BufferVal.h"
#include "DeviceVal.h"
#include "MemoryVal.h"

using namespace nri;

BufferVal::~BufferVal() {
    if (m_Memory != nullptr)
        m_Memory->UnbindBuffer(*this);
}

void BufferVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetBufferDebugName(*GetImpl(), name);
}

void BufferVal::GetMemoryInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    GetCoreInterface().GetBufferMemoryInfo(*GetImpl(), memoryLocation, memoryDesc);
    m_Device.RegisterMemoryType(memoryDesc.type, memoryLocation);
}

void* BufferVal::Map(uint64_t offset, uint64_t size) {
    RETURN_ON_FAILURE(&m_Device, m_IsBoundToMemory, nullptr, "MapBuffer: the buffer is not bound to memory");
    RETURN_ON_FAILURE(&m_Device, !m_IsMapped, nullptr, "MapBuffer: the buffer is already mapped (D3D11 doesn't support nested calls)");

    m_IsMapped = true;

    return GetCoreInterface().MapBuffer(*GetImpl(), offset, size);
}

void BufferVal::Unmap() {
    RETURN_ON_FAILURE(&m_Device, m_IsMapped, ReturnVoid(), "UnmapBuffer: the buffer is not mapped");

    m_IsMapped = false;

    GetCoreInterface().UnmapBuffer(*GetImpl());
}

#include "BufferVal.hpp"
