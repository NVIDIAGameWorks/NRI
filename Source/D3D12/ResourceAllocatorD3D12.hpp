// © 2021 NVIDIA Corporation

Result DeviceD3D12::CreateVma() {
    if (m_Vma)
        return Result::SUCCESS;

    D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
    allocatorDesc.pDevice = m_Device;
    allocatorDesc.pAdapter = m_Adapter;
    allocatorDesc.pAllocationCallbacks = m_AllocationCallbackPtr;
    allocatorDesc.Flags = (D3D12MA::ALLOCATOR_FLAGS)(D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED | D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DONT_PREFER_SMALL_BUFFERS_COMMITTED);
    allocatorDesc.PreferredBlockSize = VMA_PREFERRED_BLOCK_SIZE;

    HRESULT hr = D3D12MA::CreateAllocator(&allocatorDesc, &m_Vma);
    RETURN_ON_BAD_HRESULT(this, hr, "D3D12MA::CreateAllocator");

    return Result::SUCCESS;
}

Result BufferD3D12::Create(const AllocateBufferDesc& bufferDesc) {
    Result nriResult = m_Device.CreateVma();
    if (nriResult != Result::SUCCESS)
        return nriResult;

    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = GetHeapType(bufferDesc.memoryLocation);
    allocationDesc.Flags = (D3D12MA::ALLOCATION_FLAGS)(D3D12MA::ALLOCATION_FLAG_CAN_ALIAS | D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY);

#ifdef NRI_USE_AGILITY_SDK
    if (m_Device.GetVersion() >= 10) {
        D3D12_RESOURCE_DESC1 desc1 = {};
        GetResourceDesc((D3D12_RESOURCE_DESC*)&desc1, bufferDesc.desc);

        uint32_t castableFormatNum = 0;
        DXGI_FORMAT* castableFormats = nullptr; // TODO: add castable formats, see options12.RelaxedFormatCastingSupported

        HRESULT hr = m_Device.GetVma()->CreateResource3(&allocationDesc, &desc1, D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, castableFormatNum, castableFormats, &m_VmaAllocation, IID_PPV_ARGS(&m_Buffer));

        RETURN_ON_BAD_HRESULT(&m_Device, hr, "D3D12MA::CreateResource3()");
    } else
#endif
    {
        D3D12_RESOURCE_DESC desc = {};
        GetResourceDesc(&desc, bufferDesc.desc);

        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
        if (bufferDesc.memoryLocation == MemoryLocation::HOST_UPLOAD || bufferDesc.memoryLocation == MemoryLocation::DEVICE_UPLOAD)
            initialState |= D3D12_RESOURCE_STATE_GENERIC_READ;
        else if (bufferDesc.memoryLocation == MemoryLocation::HOST_READBACK)
            initialState |= D3D12_RESOURCE_STATE_COPY_DEST;

        if (bufferDesc.desc.usage & BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE)
            initialState |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

        HRESULT hr = m_Device.GetVma()->CreateResource(&allocationDesc, &desc, initialState, nullptr, &m_VmaAllocation, IID_PPV_ARGS(&m_Buffer));

        RETURN_ON_BAD_HRESULT(&m_Device, hr, "D3D12MA::CreateResource3()");
    }

    // Priority
    D3D12_RESIDENCY_PRIORITY residencyPriority = (D3D12_RESIDENCY_PRIORITY)ConvertPriority(bufferDesc.memoryPriority);
    if (m_Device.GetVersion() >= 1 && residencyPriority != 0) {
        ID3D12Pageable* obj = m_Buffer.GetInterface();
        HRESULT hr = m_Device->SetResidencyPriority(1, &obj, &residencyPriority);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device1::SetResidencyPriority()");
    }

    m_Desc = bufferDesc.desc;

    return Result::SUCCESS;
}

Result TextureD3D12::Create(const AllocateTextureDesc& textureDesc) {
    Result nriResult = m_Device.CreateVma();
    if (nriResult != Result::SUCCESS)
        return nriResult;

    D3D12_CLEAR_VALUE clearValue = {GetDxgiFormat(textureDesc.desc.format).typed};

    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = GetHeapType(textureDesc.memoryLocation);
    allocationDesc.Flags = (D3D12MA::ALLOCATION_FLAGS)(D3D12MA::ALLOCATION_FLAG_CAN_ALIAS | D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY);

#ifdef NRI_USE_AGILITY_SDK
    if (m_Device.GetVersion() >= 10) {
        D3D12_RESOURCE_DESC1 desc1 = {};
        GetResourceDesc((D3D12_RESOURCE_DESC*)&desc1, textureDesc.desc);

        bool isRenderableSurface = desc1.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
        uint32_t castableFormatNum = 0;
        DXGI_FORMAT* castableFormats = nullptr; // TODO: add castable formats, see options12.RelaxedFormatCastingSupported

        HRESULT hr = m_Device.GetVma()->CreateResource3(&allocationDesc, &desc1, D3D12_BARRIER_LAYOUT_COMMON, isRenderableSurface ? &clearValue : nullptr, castableFormatNum,
            castableFormats, &m_VmaAllocation, IID_PPV_ARGS(&m_Texture));

        RETURN_ON_BAD_HRESULT(&m_Device, hr, "D3D12MA::CreateResource3()");
    } else
#endif
    {
        D3D12_RESOURCE_DESC desc = {};
        GetResourceDesc(&desc, textureDesc.desc);

        bool isRenderableSurface = desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        HRESULT hr = m_Device.GetVma()->CreateResource(&allocationDesc, &desc, D3D12_RESOURCE_STATE_COMMON, isRenderableSurface ? &clearValue : nullptr, &m_VmaAllocation, IID_PPV_ARGS(&m_Texture));

        RETURN_ON_BAD_HRESULT(&m_Device, hr, "D3D12MA::CreateResource3()");
    }

    // Priority
    D3D12_RESIDENCY_PRIORITY residencyPriority = (D3D12_RESIDENCY_PRIORITY)ConvertPriority(textureDesc.memoryPriority);
    if (m_Device.GetVersion() >= 1 && residencyPriority != 0) {
        ID3D12Pageable* obj = m_Texture.GetInterface();
        HRESULT hr = m_Device->SetResidencyPriority(1, &obj, &residencyPriority);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device1::SetResidencyPriority()");
    }

    m_Desc = textureDesc.desc;

    return Result::SUCCESS;
}

Result AccelerationStructureD3D12::Create(const AllocateAccelerationStructureDesc& accelerationStructureDesc) {
    if (m_Device.GetVersion() < 5)
        return Result::UNSUPPORTED;

    Result nriResult = m_Device.CreateVma();
    if (nriResult != Result::SUCCESS)
        return nriResult;

    m_Device.GetAccelerationStructurePrebuildInfo(accelerationStructureDesc.desc, m_PrebuildInfo);

    AllocateBufferDesc bufferDesc = {};
    bufferDesc.memoryLocation = accelerationStructureDesc.memoryLocation;
    bufferDesc.memoryPriority = accelerationStructureDesc.memoryPriority;
    bufferDesc.desc.size = m_PrebuildInfo.ResultDataMaxSizeInBytes;
    bufferDesc.desc.usage = BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE;

    return m_Device.CreateImplementation<BufferD3D12>((Buffer*&)m_Buffer, bufferDesc);
}
