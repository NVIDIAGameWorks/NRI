// © 2021 NVIDIA Corporation

#include "SharedExternal.h"
#include "DeviceBase.h"
#include "DeviceVal.h"
#include "SharedVal.h"
#include "MemoryVal.h"
#include "TextureVal.h"

using namespace nri;

TextureVal::~TextureVal() {
    if (m_Memory != nullptr)
        m_Memory->UnbindTexture(*this);
}

void TextureVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetTextureDebugName(*GetImpl(), name);
}

void TextureVal::GetMemoryInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    GetCoreInterface().GetTextureMemoryInfo(*GetImpl(), memoryLocation, memoryDesc);
    m_Device.RegisterMemoryType(memoryDesc.type, memoryLocation);
}

#include "TextureVal.hpp"
