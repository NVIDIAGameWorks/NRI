/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "SharedExternal.h"
#include "DeviceBase.h"
#include "DeviceVal.h"
#include "SharedVal.h"
#include "MemoryVal.h"
#include "BufferVal.h"

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

    RETURN_ON_FAILURE(&m_Device, !m_IsMapped, nullptr, "MapBuffer: the buffer is already mapped");

    m_IsMapped = true;

    return GetCoreInterface().MapBuffer(*GetImpl(), offset, size);
}

void BufferVal::Unmap() {
    RETURN_ON_FAILURE(&m_Device, m_IsMapped, ReturnVoid(), "UnmapBuffer: the buffer is not mapped");

    m_IsMapped = false;

    GetCoreInterface().UnmapBuffer(*GetImpl());
}

#include "BufferVal.hpp"
