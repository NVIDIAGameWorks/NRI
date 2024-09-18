// © 2021 NVIDIA Corporation

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, ResourceType resourceType)
    : DeviceObjectVal(device, descriptor)
    , m_ResourceType(resourceType) {
}

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, const BufferViewDesc& bufferViewDesc)
    : DeviceObjectVal(device, descriptor)
    , m_ResourceType(ResourceType::BUFFER) {
    switch (bufferViewDesc.viewType) {
        case BufferViewType::CONSTANT:
            m_ResourceViewType = ResourceViewType::CONSTANT_BUFFER;
            break;
        case BufferViewType::SHADER_RESOURCE:
            m_ResourceViewType = ResourceViewType::SHADER_RESOURCE;
            break;
        case BufferViewType::SHADER_RESOURCE_STORAGE:
            m_ResourceViewType = ResourceViewType::SHADER_RESOURCE_STORAGE;
            break;
        default:
            CHECK(false, "unexpected BufferView");
            break;
    }
}

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, const Texture1DViewDesc& textureViewDesc)
    : DeviceObjectVal(device, descriptor)
    , m_ResourceType(ResourceType::TEXTURE) {
    switch (textureViewDesc.viewType) {
        case Texture1DViewType::SHADER_RESOURCE_1D:
        case Texture1DViewType::SHADER_RESOURCE_1D_ARRAY:
            m_ResourceViewType = ResourceViewType::SHADER_RESOURCE;
            break;
        case Texture1DViewType::SHADER_RESOURCE_STORAGE_1D:
        case Texture1DViewType::SHADER_RESOURCE_STORAGE_1D_ARRAY:
            m_ResourceViewType = ResourceViewType::SHADER_RESOURCE_STORAGE;
            break;
        case Texture1DViewType::COLOR_ATTACHMENT:
            m_ResourceViewType = ResourceViewType::COLOR_ATTACHMENT;
            break;
        case Texture1DViewType::DEPTH_STENCIL_ATTACHMENT:
        case Texture1DViewType::DEPTH_READONLY_STENCIL_ATTACHMENT:
        case Texture1DViewType::DEPTH_ATTACHMENT_STENCIL_READONLY:
        case Texture1DViewType::DEPTH_STENCIL_READONLY:
            m_ResourceViewType = ResourceViewType::DEPTH_STENCIL_ATTACHMENT;
            m_IsDepthReadonly = textureViewDesc.viewType == Texture1DViewType::DEPTH_READONLY_STENCIL_ATTACHMENT || textureViewDesc.viewType == Texture1DViewType::DEPTH_STENCIL_READONLY;
            m_IsStencilReadonly = textureViewDesc.viewType == Texture1DViewType::DEPTH_ATTACHMENT_STENCIL_READONLY || textureViewDesc.viewType == Texture1DViewType::DEPTH_STENCIL_READONLY;
            break;
        default:
            CHECK(false, "unexpected TextureView");
            break;
    }
}

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, const Texture2DViewDesc& textureViewDesc)
    : DeviceObjectVal(device, descriptor)
    , m_ResourceType(ResourceType::TEXTURE) {
    switch (textureViewDesc.viewType) {
        case Texture2DViewType::SHADER_RESOURCE_2D:
        case Texture2DViewType::SHADER_RESOURCE_2D_ARRAY:
        case Texture2DViewType::SHADER_RESOURCE_CUBE:
        case Texture2DViewType::SHADER_RESOURCE_CUBE_ARRAY:
            m_ResourceViewType = ResourceViewType::SHADER_RESOURCE;
            break;
        case Texture2DViewType::SHADER_RESOURCE_STORAGE_2D:
        case Texture2DViewType::SHADER_RESOURCE_STORAGE_2D_ARRAY:
            m_ResourceViewType = ResourceViewType::SHADER_RESOURCE_STORAGE;
            break;
        case Texture2DViewType::COLOR_ATTACHMENT:
            m_ResourceViewType = ResourceViewType::COLOR_ATTACHMENT;
            break;
        case Texture2DViewType::DEPTH_STENCIL_ATTACHMENT:
        case Texture2DViewType::DEPTH_READONLY_STENCIL_ATTACHMENT:
        case Texture2DViewType::DEPTH_ATTACHMENT_STENCIL_READONLY:
        case Texture2DViewType::DEPTH_STENCIL_READONLY:
            m_ResourceViewType = ResourceViewType::DEPTH_STENCIL_ATTACHMENT;
            m_IsDepthReadonly = textureViewDesc.viewType == Texture2DViewType::DEPTH_READONLY_STENCIL_ATTACHMENT || textureViewDesc.viewType == Texture2DViewType::DEPTH_STENCIL_READONLY;
            m_IsStencilReadonly = textureViewDesc.viewType == Texture2DViewType::DEPTH_ATTACHMENT_STENCIL_READONLY || textureViewDesc.viewType == Texture2DViewType::DEPTH_STENCIL_READONLY;
            break;
        case Texture2DViewType::SHADING_RATE_ATTACHMENT:
            m_ResourceViewType = ResourceViewType::SHADING_RATE_ATTACHMENT;
            break;
        default:
            CHECK(false, "unexpected 'viewType'");
            break;
    }
}

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, const Texture3DViewDesc& textureViewDesc)
    : DeviceObjectVal(device, descriptor)
    , m_ResourceType(ResourceType::TEXTURE) {
    switch (textureViewDesc.viewType) {
        case Texture3DViewType::SHADER_RESOURCE_3D:
            m_ResourceViewType = ResourceViewType::SHADER_RESOURCE;
            break;
        case Texture3DViewType::SHADER_RESOURCE_STORAGE_3D:
            m_ResourceViewType = ResourceViewType::SHADER_RESOURCE_STORAGE;
            break;
        case Texture3DViewType::COLOR_ATTACHMENT:
            m_ResourceViewType = ResourceViewType::COLOR_ATTACHMENT;
            break;
        default:
            CHECK(false, "unexpected TextureView");
            break;
    }
}

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor)
    : DeviceObjectVal(device, descriptor)
    , m_ResourceType(ResourceType::SAMPLER) {
}

NRI_INLINE void DescriptorVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetDescriptorDebugName(*GetImpl(), name);
}
