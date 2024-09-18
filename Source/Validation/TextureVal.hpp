// © 2021 NVIDIA Corporation

TextureVal::~TextureVal() {
    if (m_Memory != nullptr)
        m_Memory->UnbindTexture(*this);
}

NRI_INLINE void TextureVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetTextureDebugName(*GetImpl(), name);
}
