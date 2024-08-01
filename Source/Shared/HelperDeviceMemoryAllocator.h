#pragma once

template <typename U, typename T>
using Map = std::map<U, T, std::less<U>, StdAllocator<std::pair<const U, T>>>;

struct HelperDeviceMemoryAllocator {
    HelperDeviceMemoryAllocator(const nri::CoreInterface& NRI, nri::Device& device);

    uint32_t CalculateAllocationNumber(const nri::ResourceGroupDesc& resourceGroupDesc);
    nri::Result AllocateAndBindMemory(const nri::ResourceGroupDesc& resourceGroupDesc, nri::Memory** allocations);

private:
    struct MemoryHeap {
        MemoryHeap(nri::MemoryType memoryType, const StdAllocator<uint8_t>& stdAllocator);

        Vector<nri::Buffer*> buffers;
        Vector<uint64_t> bufferOffsets;
        Vector<nri::Texture*> textures;
        Vector<uint64_t> textureOffsets;
        uint64_t size;
        nri::MemoryType type;
    };

    nri::Result TryToAllocateAndBindMemory(const nri::ResourceGroupDesc& resourceGroupDesc, nri::Memory** allocations, size_t& allocationNum);
    nri::Result ProcessDedicatedResources(nri::MemoryLocation memoryLocation, nri::Memory** allocations, size_t& allocationNum);
    MemoryHeap& FindOrCreateHeap(nri::MemoryDesc& memoryDesc, uint64_t preferredMemorySize);
    void GroupByMemoryType(nri::MemoryLocation memoryLocation, const nri::ResourceGroupDesc& resourceGroupDesc);
    void FillMemoryBindingDescs(nri::Buffer* const* buffers, const uint64_t* bufferOffsets, uint32_t bufferNum, nri::Memory& memory);
    void FillMemoryBindingDescs(nri::Texture* const* texture, const uint64_t* textureOffsets, uint32_t textureNum, nri::Memory& memory);

    const nri::CoreInterface& m_NRI;
    nri::Device& m_Device;

    Vector<MemoryHeap> m_Heaps;
    Vector<nri::Buffer*> m_DedicatedBuffers;
    Vector<nri::Texture*> m_DedicatedTextures;
    Vector<nri::BufferMemoryBindingDesc> m_BufferBindingDescs;
    Vector<nri::TextureMemoryBindingDesc> m_TextureBindingDescs;
};
