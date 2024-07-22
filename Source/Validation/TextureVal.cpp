// © 2021 NVIDIA Corporation

#include "SharedExternal.h"
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

#include "TextureVal.hpp"
