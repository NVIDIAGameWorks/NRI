// © 2021 NVIDIA Corporation

TextureVK::~TextureVK() {
    if (m_OwnsNativeObjects) {
        const auto& vk = m_Device.GetDispatchTable();

        if (m_VmaAllocation)
            DestroyVma();
        else
            vk.DestroyImage(m_Device, m_Handle, m_Device.GetAllocationCallbacks());
    }
}

Result TextureVK::Create(const TextureDesc& textureDesc) {
    VkImageCreateInfo info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    m_Device.FillCreateInfo(textureDesc, info);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateImage(m_Device, &info, m_Device.GetAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateImage returned %d", (int32_t)result);

    m_Desc = textureDesc;

    return Result::SUCCESS;
}

Result TextureVK::Create(const TextureVKDesc& textureDesc) {
    if (!textureDesc.vkImage)
        return Result::INVALID_ARGUMENT;

    m_Desc.type = GetTextureType((VkImageType)textureDesc.vkImageType);
    m_Desc.usageMask = (TextureUsageBits)(-1); // TODO: it's not right...
    m_Desc.format = VKFormatToNRIFormat((VkFormat)textureDesc.vkFormat);
    m_Desc.width = textureDesc.width;
    m_Desc.height = textureDesc.height;
    m_Desc.depth = textureDesc.depth;
    m_Desc.mipNum = textureDesc.mipNum;
    m_Desc.layerNum = textureDesc.layerNum;
    m_Desc.sampleNum = textureDesc.sampleNum;

    m_OwnsNativeObjects = false;
    m_Handle = (VkImage)textureDesc.vkImage;

    return Result::SUCCESS;
}

VkImageAspectFlags TextureVK::GetImageAspectFlags() const {
    return ::GetImageAspectFlags(m_Desc.format);
}

NRI_INLINE void TextureVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_IMAGE, (uint64_t)m_Handle, name);
}
