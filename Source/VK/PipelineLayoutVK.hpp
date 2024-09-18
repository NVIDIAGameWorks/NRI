// © 2021 NVIDIA Corporation

BindingInfo::BindingInfo(StdAllocator<uint8_t>& allocator)
    : hasVariableDescriptorNum(allocator)
    , descriptorSetRangeDescs(allocator)
    , dynamicConstantBufferDescs(allocator)
    , descriptorSetDescs(allocator)
    , pushConstantBindings(allocator)
    , pushDescriptorBindings(allocator) {
}

PipelineLayoutVK::~PipelineLayoutVK() {
    const auto& vk = m_Device.GetDispatchTable();
    const auto allocationCallbacks = m_Device.GetAllocationCallbacks();

    if (m_Handle)
        vk.DestroyPipelineLayout(m_Device, m_Handle, allocationCallbacks);

    for (auto& handle : m_DescriptorSetLayouts)
        vk.DestroyDescriptorSetLayout(m_Device, handle, allocationCallbacks);
}

Result PipelineLayoutVK::Create(const PipelineLayoutDesc& pipelineLayoutDesc) {
    // Binding point
    if (pipelineLayoutDesc.shaderStages & StageBits::GRAPHICS_SHADERS)
        m_PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    else if (pipelineLayoutDesc.shaderStages & StageBits::COMPUTE_SHADER)
        m_PipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
    else if (pipelineLayoutDesc.shaderStages & StageBits::RAY_TRACING_SHADERS)
        m_PipelineBindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;

    // Create set layouts
    uint32_t setNum = 0;

    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        const DescriptorSetDesc& descriptorSetDesc = pipelineLayoutDesc.descriptorSets[i];

        // Non-push
        VkDescriptorSetLayout descriptorSetLayout = CreateSetLayout(descriptorSetDesc, pipelineLayoutDesc.ignoreGlobalSPIRVOffsets, false);
        m_DescriptorSetLayouts.push_back(descriptorSetLayout);

        setNum = std::max(setNum, descriptorSetDesc.registerSpace);
    }

    if (m_Device.GetDesc().rootDescriptorMaxNum) {
        for (uint32_t i = 0; i < pipelineLayoutDesc.rootDescriptorSetNum; i++) {
            const RootDescriptorSetDesc& rootDescriptorDesc = pipelineLayoutDesc.rootDescriptorSets[i];

            DescriptorRangeDesc range = {};
            range.baseRegisterIndex = rootDescriptorDesc.registerIndex;
            range.descriptorNum = 1;
            range.descriptorType = rootDescriptorDesc.descriptorType;
            range.shaderStages = rootDescriptorDesc.shaderStages;

            DescriptorSetDesc descriptorSetDesc = {};
            descriptorSetDesc.registerSpace = rootDescriptorDesc.registerSpace;
            descriptorSetDesc.ranges = &range;
            descriptorSetDesc.rangeNum = 1;

            // Push
            VkDescriptorSetLayout descriptorSetLayout = CreateSetLayout(descriptorSetDesc, pipelineLayoutDesc.ignoreGlobalSPIRVOffsets, true);
            m_DescriptorSetLayouts.push_back(descriptorSetLayout);

            setNum = std::max(setNum, descriptorSetDesc.registerSpace);
        }
    }

    setNum++;

    // Allocate temp memory for ALL "register spaces" making the entire range consecutive (thanks VK API!)
    Scratch<VkDescriptorSetLayout> descriptorSetLayouts = AllocateScratch(m_Device, VkDescriptorSetLayout, setNum);

    if (setNum != pipelineLayoutDesc.descriptorSetNum + pipelineLayoutDesc.rootDescriptorSetNum) {
        // Create "dummy" set layout (needed only if "register space" indices are not consecutive)
        VkDescriptorSetLayout dummyDescriptorSetLayout = CreateSetLayout({}, pipelineLayoutDesc.ignoreGlobalSPIRVOffsets, true); // created as "push"
        m_DescriptorSetLayouts.push_back(dummyDescriptorSetLayout);

        for (uint32_t i = 0; i < setNum; i++)
            descriptorSetLayouts[i] = dummyDescriptorSetLayout;
    }

    // Populate descriptor set layouts in proper order
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        uint32_t setIndex = pipelineLayoutDesc.descriptorSets[i].registerSpace;
        descriptorSetLayouts[setIndex] = m_DescriptorSetLayouts[i];
    }

    for (uint32_t i = 0; i < pipelineLayoutDesc.rootDescriptorSetNum; i++) {
        uint32_t setIndex = pipelineLayoutDesc.rootDescriptorSets[i].registerSpace;
        descriptorSetLayouts[setIndex] = m_DescriptorSetLayouts[pipelineLayoutDesc.descriptorSetNum + i];
    }

    // Root constants
    Scratch<VkPushConstantRange> pushConstantRanges = AllocateScratch(m_Device, VkPushConstantRange, pipelineLayoutDesc.rootConstantNum);

    uint32_t offset = 0;
    for (uint32_t i = 0; i < pipelineLayoutDesc.rootConstantNum; i++) {
        const RootConstantDesc& pushConstantDesc = pipelineLayoutDesc.rootConstants[i];

        VkPushConstantRange& range = pushConstantRanges[i];
        range = {};
        range.stageFlags = GetShaderStageFlags(pushConstantDesc.shaderStages);
        range.offset = offset;
        range.size = pushConstantDesc.size;

        offset += pushConstantDesc.size;
    }

    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutCreateInfo.setLayoutCount = setNum;
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;
    pipelineLayoutCreateInfo.pushConstantRangeCount = pipelineLayoutDesc.rootConstantNum;
    pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreatePipelineLayout(m_Device, &pipelineLayoutCreateInfo, m_Device.GetAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, Result::FAILURE, "vkCreatePipelineLayout returned %d", (int32_t)result);

    FillBindingInfo(pipelineLayoutDesc);

    return Result::SUCCESS;
}

VkDescriptorSetLayout PipelineLayoutVK::CreateSetLayout(const DescriptorSetDesc& descriptorSetDesc, bool ignoreGlobalSPIRVOffsets, bool isPush) {
    // Binding offsets
    SPIRVBindingOffsets spirvBindingOffsets = {};
    if (!ignoreGlobalSPIRVOffsets)
        spirvBindingOffsets = m_Device.GetSPIRVBindingOffsets();

    std::array<uint32_t, (size_t)DescriptorType::MAX_NUM> bindingOffsets = {};
    bindingOffsets[(uint32_t)DescriptorType::SAMPLER] = spirvBindingOffsets.samplerOffset;
    bindingOffsets[(uint32_t)DescriptorType::CONSTANT_BUFFER] = spirvBindingOffsets.constantBufferOffset;
    bindingOffsets[(uint32_t)DescriptorType::TEXTURE] = spirvBindingOffsets.textureOffset;
    bindingOffsets[(uint32_t)DescriptorType::STORAGE_TEXTURE] = spirvBindingOffsets.storageTextureAndBufferOffset;
    bindingOffsets[(uint32_t)DescriptorType::BUFFER] = spirvBindingOffsets.textureOffset;
    bindingOffsets[(uint32_t)DescriptorType::STORAGE_BUFFER] = spirvBindingOffsets.storageTextureAndBufferOffset;
    bindingOffsets[(uint32_t)DescriptorType::STRUCTURED_BUFFER] = spirvBindingOffsets.textureOffset;
    bindingOffsets[(uint32_t)DescriptorType::STORAGE_STRUCTURED_BUFFER] = spirvBindingOffsets.storageTextureAndBufferOffset;
    bindingOffsets[(uint32_t)DescriptorType::ACCELERATION_STRUCTURE] = spirvBindingOffsets.textureOffset;

    // Count
    uint32_t bindingMaxNum = descriptorSetDesc.dynamicConstantBufferNum;
    for (uint32_t i = 0; i < descriptorSetDesc.rangeNum; i++) {
        const DescriptorRangeDesc& range = descriptorSetDesc.ranges[i];
        bool isArray = range.flags & (DescriptorRangeBits::ARRAY | DescriptorRangeBits::VARIABLE_SIZED_ARRAY);
        bindingMaxNum += isArray ? 1 : range.descriptorNum;
    }

    // Allocate scratch
    Scratch<VkDescriptorSetLayoutBinding> bindingsScratch = AllocateScratch(m_Device, VkDescriptorSetLayoutBinding, bindingMaxNum);
    Scratch<VkDescriptorBindingFlags> bindingFlagsScratch = AllocateScratch(m_Device, VkDescriptorBindingFlags, bindingMaxNum);
    VkDescriptorSetLayoutBinding* bindingsBegin = bindingsScratch;
    VkDescriptorBindingFlags* bindingFlagsBegin = bindingFlagsScratch;

    VkDescriptorSetLayoutBinding* bindings = bindingsScratch;
    VkDescriptorBindingFlags* bindingFlags = bindingFlagsScratch;

    // Descriptor bindings
    for (uint32_t i = 0; i < descriptorSetDesc.rangeNum; i++) {
        const DescriptorRangeDesc& range = descriptorSetDesc.ranges[i];
        uint32_t baseBindingIndex = range.baseRegisterIndex + bindingOffsets[(uint32_t)range.descriptorType];

        VkDescriptorBindingFlags flags = (range.flags & DescriptorRangeBits::PARTIALLY_BOUND) ? VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT : 0;
        uint32_t descriptorNum = 1;

        bool isArray = range.flags & (DescriptorRangeBits::ARRAY | DescriptorRangeBits::VARIABLE_SIZED_ARRAY);
        if (isArray) {
            if (range.flags & DescriptorRangeBits::VARIABLE_SIZED_ARRAY)
                flags |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
        } else
            descriptorNum = range.descriptorNum;

        for (uint32_t j = 0; j < descriptorNum; j++) {
            *bindingFlags++ = flags;

            VkDescriptorSetLayoutBinding& descriptorBinding = *bindings++;
            descriptorBinding = {};
            descriptorBinding.descriptorType = GetDescriptorType(range.descriptorType);
            descriptorBinding.stageFlags = GetShaderStageFlags(range.shaderStages);
            descriptorBinding.binding = baseBindingIndex + j;
            descriptorBinding.descriptorCount = isArray ? range.descriptorNum : 1;
        }
    }

    // Dynamic constant buffer bindings
    for (uint32_t i = 0; i < descriptorSetDesc.dynamicConstantBufferNum; i++) {
        const DynamicConstantBufferDesc& buffer = descriptorSetDesc.dynamicConstantBuffers[i];

        *bindingFlags++ = 0;

        VkDescriptorSetLayoutBinding& descriptorBinding = *bindings++;
        descriptorBinding = {};
        descriptorBinding.binding = buffer.registerIndex + bindingOffsets[(uint32_t)DescriptorType::CONSTANT_BUFFER];
        descriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorBinding.descriptorCount = 1;
        descriptorBinding.stageFlags = GetShaderStageFlags(buffer.shaderStages);
    }

    uint32_t bindingNum = uint32_t(bindings - bindingsBegin);

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    bindingFlagsInfo.bindingCount = bindingNum;
    bindingFlagsInfo.pBindingFlags = bindingFlagsBegin;

    VkDescriptorSetLayoutCreateInfo info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.pNext = m_Device.m_IsSupported.descriptorIndexing ? &bindingFlagsInfo : nullptr;
    info.bindingCount = bindingNum;
    info.pBindings = bindingsBegin;
    if (m_Device.GetDesc().rootDescriptorMaxNum)
        info.flags = isPush ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR : 0;

    VkDescriptorSetLayout handle = VK_NULL_HANDLE;
    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateDescriptorSetLayout(m_Device, &info, m_Device.GetAllocationCallbacks(), &handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, 0, "vkCreateDescriptorSetLayout returned %d", (int32_t)result);

    return handle;
}

void PipelineLayoutVK::FillBindingInfo(const PipelineLayoutDesc& pipelineLayoutDesc) {
    // Binding offsets
    SPIRVBindingOffsets spirvBindingOffsets = {};
    if (!pipelineLayoutDesc.ignoreGlobalSPIRVOffsets)
        spirvBindingOffsets = m_Device.GetSPIRVBindingOffsets();

    std::array<uint32_t, (size_t)DescriptorType::MAX_NUM> bindingOffsets = {};
    bindingOffsets[(uint32_t)DescriptorType::SAMPLER] = spirvBindingOffsets.samplerOffset;
    bindingOffsets[(uint32_t)DescriptorType::CONSTANT_BUFFER] = spirvBindingOffsets.constantBufferOffset;
    bindingOffsets[(uint32_t)DescriptorType::TEXTURE] = spirvBindingOffsets.textureOffset;
    bindingOffsets[(uint32_t)DescriptorType::STORAGE_TEXTURE] = spirvBindingOffsets.storageTextureAndBufferOffset;
    bindingOffsets[(uint32_t)DescriptorType::BUFFER] = spirvBindingOffsets.textureOffset;
    bindingOffsets[(uint32_t)DescriptorType::STORAGE_BUFFER] = spirvBindingOffsets.storageTextureAndBufferOffset;
    bindingOffsets[(uint32_t)DescriptorType::STRUCTURED_BUFFER] = spirvBindingOffsets.textureOffset;
    bindingOffsets[(uint32_t)DescriptorType::STORAGE_STRUCTURED_BUFFER] = spirvBindingOffsets.storageTextureAndBufferOffset;
    bindingOffsets[(uint32_t)DescriptorType::ACCELERATION_STRUCTURE] = spirvBindingOffsets.textureOffset;

    // Count
    size_t rangeNum = 0;
    size_t dynamicConstantBufferNum = 0;
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        rangeNum += pipelineLayoutDesc.descriptorSets[i].rangeNum;
        dynamicConstantBufferNum += pipelineLayoutDesc.descriptorSets[i].dynamicConstantBufferNum;
    }

    // Copy descriptor set descs with dependencies
    m_BindingInfo.descriptorSetDescs.insert(m_BindingInfo.descriptorSetDescs.begin(), pipelineLayoutDesc.descriptorSets, pipelineLayoutDesc.descriptorSets + pipelineLayoutDesc.descriptorSetNum);
    m_BindingInfo.hasVariableDescriptorNum.resize(pipelineLayoutDesc.descriptorSetNum);
    m_BindingInfo.descriptorSetRangeDescs.reserve(rangeNum);
    m_BindingInfo.dynamicConstantBufferDescs.reserve(dynamicConstantBufferNum);

    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        const DescriptorSetDesc& descriptorSetDesc = pipelineLayoutDesc.descriptorSets[i];

        m_BindingInfo.hasVariableDescriptorNum[i] = false;
        m_BindingInfo.descriptorSetDescs[i].ranges = m_BindingInfo.descriptorSetRangeDescs.data() + m_BindingInfo.descriptorSetRangeDescs.size();
        m_BindingInfo.descriptorSetDescs[i].dynamicConstantBuffers = m_BindingInfo.dynamicConstantBufferDescs.data() + m_BindingInfo.dynamicConstantBufferDescs.size();

        // Copy descriptor range descs
        m_BindingInfo.descriptorSetRangeDescs.insert(m_BindingInfo.descriptorSetRangeDescs.end(), descriptorSetDesc.ranges, descriptorSetDesc.ranges + descriptorSetDesc.rangeNum);

        // Fix descriptor range binding offsets and check for variable descriptor num
        DescriptorRangeDesc* ranges = const_cast<DescriptorRangeDesc*>(m_BindingInfo.descriptorSetDescs[i].ranges);
        for (uint32_t j = 0; j < descriptorSetDesc.rangeNum; j++) {
            ranges[j].baseRegisterIndex += bindingOffsets[(uint32_t)descriptorSetDesc.ranges[j].descriptorType];

            if (m_Device.m_IsSupported.descriptorIndexing && (descriptorSetDesc.ranges[j].flags & DescriptorRangeBits::VARIABLE_SIZED_ARRAY))
                m_BindingInfo.hasVariableDescriptorNum[i] = true;
        }

        // Copy dynamic constant buffer descs
        m_BindingInfo.dynamicConstantBufferDescs.insert(m_BindingInfo.dynamicConstantBufferDescs.end(), descriptorSetDesc.dynamicConstantBuffers, descriptorSetDesc.dynamicConstantBuffers + descriptorSetDesc.dynamicConstantBufferNum);

        // Copy dynamic constant buffer binding offsets
        DynamicConstantBufferDesc* dynamicConstantBuffers = const_cast<DynamicConstantBufferDesc*>(m_BindingInfo.descriptorSetDescs[i].dynamicConstantBuffers);
        for (uint32_t j = 0; j < descriptorSetDesc.dynamicConstantBufferNum; j++)
            dynamicConstantBuffers[j].registerIndex += bindingOffsets[(uint32_t)DescriptorType::CONSTANT_BUFFER];
    }

    // Copy root constant bindings
    m_BindingInfo.pushConstantBindings.resize(pipelineLayoutDesc.rootConstantNum);
    for (uint32_t i = 0, offset = 0; i < pipelineLayoutDesc.rootConstantNum; i++) {
        m_BindingInfo.pushConstantBindings[i] = {GetShaderStageFlags(pipelineLayoutDesc.rootConstants[i].shaderStages), offset};
        offset += pipelineLayoutDesc.rootConstants[i].size;
    }

    // Copy root descriptor bindings
    for (uint32_t i = 0; i < pipelineLayoutDesc.rootDescriptorSetNum; i++) {
        const RootDescriptorSetDesc& rootDescriptorDesc = pipelineLayoutDesc.rootDescriptorSets[i];
        uint32_t registerIndex = rootDescriptorDesc.registerIndex + bindingOffsets[(uint32_t)rootDescriptorDesc.descriptorType];

        m_BindingInfo.pushDescriptorBindings.push_back({rootDescriptorDesc.registerSpace, registerIndex});
    }
}

NRI_INLINE void PipelineLayoutVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)m_Handle, name);
}
