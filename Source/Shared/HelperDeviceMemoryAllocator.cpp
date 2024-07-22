#include "SharedExternal.h"

#include "HelperDeviceMemoryAllocator.h"

using namespace nri;

HelperDeviceMemoryAllocator::MemoryTypeGroup::MemoryTypeGroup(const StdAllocator<uint8_t>& stdAllocator) :
    buffers(stdAllocator), bufferOffsets(stdAllocator), textures(stdAllocator), textureOffsets(stdAllocator), memoryOffset(0) {
}

HelperDeviceMemoryAllocator::HelperDeviceMemoryAllocator(const CoreInterface& NRI, Device& device) :
    m_NRI(NRI),
    m_Device(device),
    m_Map(((DeviceBase&)device).GetStdAllocator()),
    m_DedicatedBuffers(((DeviceBase&)device).GetStdAllocator()),
    m_DedicatedTextures(((DeviceBase&)device).GetStdAllocator()),
    m_BufferBindingDescs(((DeviceBase&)device).GetStdAllocator()),
    m_TextureBindingDescs(((DeviceBase&)device).GetStdAllocator()) {
}

uint32_t HelperDeviceMemoryAllocator::CalculateAllocationNumber(const ResourceGroupDesc& resourceGroupDesc) {
    m_Map.clear();
    m_DedicatedBuffers.clear();
    m_DedicatedTextures.clear();

    GroupByMemoryType(resourceGroupDesc.memoryLocation, resourceGroupDesc.buffers, resourceGroupDesc.bufferNum);
    GroupByMemoryType(resourceGroupDesc.memoryLocation, resourceGroupDesc.textures, resourceGroupDesc.textureNum);

    return uint32_t(m_Map.size()) + uint32_t(m_DedicatedBuffers.size()) + uint32_t(m_DedicatedTextures.size());
}

Result HelperDeviceMemoryAllocator::AllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations) {
    m_Map.clear();
    m_DedicatedBuffers.clear();
    m_DedicatedTextures.clear();
    m_BufferBindingDescs.clear();
    m_TextureBindingDescs.clear();

    size_t allocationNum = 0;
    const Result result = TryToAllocateAndBindMemory(resourceGroupDesc, allocations, allocationNum);

    if (result != Result::SUCCESS) {
        for (size_t i = 0; i < allocationNum; i++) {
            m_NRI.FreeMemory(*allocations[i]);
            allocations[i] = nullptr;
        }
    }

    return result;
}

Result HelperDeviceMemoryAllocator::TryToAllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations, size_t& allocationNum) {
    GroupByMemoryType(resourceGroupDesc.memoryLocation, resourceGroupDesc.buffers, resourceGroupDesc.bufferNum);
    GroupByMemoryType(resourceGroupDesc.memoryLocation, resourceGroupDesc.textures, resourceGroupDesc.textureNum);

    Result result = Result::SUCCESS;

    for (auto it = m_Map.begin(); it != m_Map.end() && result == Result::SUCCESS; ++it)
        result = ProcessMemoryTypeGroup(it->first, it->second, allocations, allocationNum);

    if (result != Result::SUCCESS)
        return result;

    result = ProcessDedicatedResources(resourceGroupDesc.memoryLocation, allocations, allocationNum);

    if (result != Result::SUCCESS)
        return result;

    result = m_NRI.BindBufferMemory(m_Device, m_BufferBindingDescs.data(), (uint32_t)m_BufferBindingDescs.size());

    if (result != Result::SUCCESS)
        return result;

    result = m_NRI.BindTextureMemory(m_Device, m_TextureBindingDescs.data(), (uint32_t)m_TextureBindingDescs.size());

    return result;
}

Result HelperDeviceMemoryAllocator::ProcessMemoryTypeGroup(MemoryType memoryType, MemoryTypeGroup& group, Memory** allocations, size_t& allocationNum) {
    Memory*& memory = allocations[allocationNum];

    const uint64_t allocationSize = group.memoryOffset;

    const Result result = m_NRI.AllocateMemory(m_Device, memoryType, allocationSize, memory);
    if (result != Result::SUCCESS)
        return result;

    FillMemoryBindingDescs(group.buffers.data(), group.bufferOffsets.data(), (uint32_t)group.buffers.size(), *memory);
    FillMemoryBindingDescs(group.textures.data(), group.textureOffsets.data(), (uint32_t)group.textures.size(), *memory);
    allocationNum++;

    return Result::SUCCESS;
}

Result HelperDeviceMemoryAllocator::ProcessDedicatedResources(MemoryLocation memoryLocation, Memory** allocations, size_t& allocationNum) {
    constexpr uint64_t zeroOffset = 0;
    MemoryDesc memoryDesc = {};

    for (size_t i = 0; i < m_DedicatedBuffers.size(); i++) {
        const BufferDesc& bufferDesc = m_NRI.GetBufferDesc(*m_DedicatedBuffers[i]);
        m_NRI.GetBufferMemoryDesc(m_Device, bufferDesc, memoryLocation, memoryDesc);

        Memory*& memory = allocations[allocationNum];

        const Result result = m_NRI.AllocateMemory(m_Device, memoryDesc.type, memoryDesc.size, memory);
        if (result != Result::SUCCESS)
            return result;

        FillMemoryBindingDescs(m_DedicatedBuffers.data() + i, &zeroOffset, 1, *memory);
        allocationNum++;
    }

    for (size_t i = 0; i < m_DedicatedTextures.size(); i++) {
        const TextureDesc& textureDesc = m_NRI.GetTextureDesc(*m_DedicatedTextures[i]);
        m_NRI.GetTextureMemoryDesc(m_Device, textureDesc, memoryLocation, memoryDesc);

        Memory*& memory = allocations[allocationNum];

        const Result result = m_NRI.AllocateMemory(m_Device, memoryDesc.type, memoryDesc.size, memory);
        if (result != Result::SUCCESS)
            return result;

        FillMemoryBindingDescs(m_DedicatedTextures.data() + i, &zeroOffset, 1, *memory);
        allocationNum++;
    }

    return Result::SUCCESS;
}

void HelperDeviceMemoryAllocator::GroupByMemoryType(MemoryLocation memoryLocation, Buffer* const* buffers, uint32_t bufferNum) {
    MemoryDesc memoryDesc = {};

    for (uint32_t i = 0; i < bufferNum; i++) {
        Buffer* buffer = buffers[i];
        const BufferDesc& bufferDesc = m_NRI.GetBufferDesc(*buffer);
        m_NRI.GetBufferMemoryDesc(m_Device, bufferDesc, memoryLocation, memoryDesc);

        if (memoryDesc.mustBeDedicated)
            m_DedicatedBuffers.push_back(buffer);
        else {
            MemoryTypeGroup& group = m_Map.try_emplace(memoryDesc.type, ((DeviceBase&)m_Device).GetStdAllocator()).first->second;

            uint64_t offset = Align(group.memoryOffset, memoryDesc.alignment);

            group.buffers.push_back(buffer);
            group.bufferOffsets.push_back(offset);
            group.memoryOffset = offset + memoryDesc.size;
        }
    }
}

void HelperDeviceMemoryAllocator::GroupByMemoryType(MemoryLocation memoryLocation, Texture* const* textures, uint32_t textureNum) {
    const DeviceDesc& deviceDesc = m_NRI.GetDeviceDesc(m_Device);

    MemoryDesc memoryDesc = {};

    for (uint32_t i = 0; i < textureNum; i++) {
        Texture* texture = textures[i];
        const TextureDesc& textureDesc = m_NRI.GetTextureDesc(*texture);
        m_NRI.GetTextureMemoryDesc(m_Device, textureDesc, memoryLocation, memoryDesc);

        if (memoryDesc.mustBeDedicated)
            m_DedicatedTextures.push_back(texture);
        else {
            MemoryTypeGroup& group = m_Map.try_emplace(memoryDesc.type, ((DeviceBase&)m_Device).GetStdAllocator()).first->second;

            if (group.textures.empty() && group.memoryOffset > 0)
                group.memoryOffset = Align(group.memoryOffset, deviceDesc.bufferTextureGranularity);

            uint64_t offset = Align(group.memoryOffset, memoryDesc.alignment);

            group.textures.push_back(texture);
            group.textureOffsets.push_back(offset);
            group.memoryOffset = offset + memoryDesc.size;
        }
    }
}

void HelperDeviceMemoryAllocator::FillMemoryBindingDescs(Buffer* const* buffers, const uint64_t* bufferOffsets, uint32_t bufferNum, Memory& memory) {
    for (uint32_t i = 0; i < bufferNum; i++) {
        BufferMemoryBindingDesc desc = {};
        desc.memory = &memory;
        desc.buffer = buffers[i];
        desc.offset = bufferOffsets[i];

        m_BufferBindingDescs.push_back(desc);
    }
}

void HelperDeviceMemoryAllocator::FillMemoryBindingDescs(Texture* const* textures, const uint64_t* textureOffsets, uint32_t textureNum, Memory& memory) {
    for (uint32_t i = 0; i < textureNum; i++) {
        TextureMemoryBindingDesc desc = {};
        desc.memory = &memory;
        desc.texture = textures[i];
        desc.offset = textureOffsets[i];

        m_TextureBindingDescs.push_back(desc);
    }
}
