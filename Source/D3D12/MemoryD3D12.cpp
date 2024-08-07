// © 2021 NVIDIA Corporation

#include "SharedD3D12.h"

#include "MemoryD3D12.h"

using namespace nri;

static inline D3D12_HEAP_TYPE GetHeapType(MemoryType memoryType) {
    return (D3D12_HEAP_TYPE)(memoryType >> 16);
}

Result MemoryD3D12::Create(const AllocateMemoryDesc& allocateMemoryDesc) {
    if (allocateMemoryDesc.priority != 0.0f) {
        float p = allocateMemoryDesc.priority * 0.5f + 0.5f;
        float level = 0.0f;

        if (p < 0.2f) {
            m_Priority = (uint32_t)D3D12_RESIDENCY_PRIORITY_MINIMUM;
            level = 0.0f;
        } else if (p < 0.4f) {
            m_Priority = (uint32_t)D3D12_RESIDENCY_PRIORITY_LOW;
            level = 0.2f;
        } else if (p < 0.6f) {
            m_Priority = (uint32_t)D3D12_RESIDENCY_PRIORITY_NORMAL;
            level = 0.4f;
        } else if (p < 0.8f) {
            m_Priority = (uint32_t)D3D12_RESIDENCY_PRIORITY_HIGH;
            level = 0.6f;
        } else {
            m_Priority = (uint32_t)D3D12_RESIDENCY_PRIORITY_MAXIMUM;
            level = 0.8f;
        }

        uint32_t bonus = uint32_t(((p - level) / 0.2f) * 65535.0f);
        if (bonus > 0xFFFF)
            bonus = 0xFFFF;

        m_Priority |= bonus;
    }

    D3D12_HEAP_DESC heapDesc = {};
    heapDesc.SizeInBytes = allocateMemoryDesc.size;
    heapDesc.Properties.Type = GetHeapType(allocateMemoryDesc.type);
    heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapDesc.Properties.CreationNodeMask = NRI_NODE_MASK;
    heapDesc.Properties.VisibleNodeMask = NRI_NODE_MASK;
    heapDesc.Alignment = 0;
    heapDesc.Flags = (allocateMemoryDesc.size ? GetHeapFlags(allocateMemoryDesc.type) : D3D12_HEAP_FLAG_NONE) | D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;

    if (!m_Device.IsDedicated(allocateMemoryDesc.type)) {
        HRESULT hr = m_Device->CreateHeap(&heapDesc, IID_PPV_ARGS(&m_Heap));
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateHeap()");

        SetPriority(m_Heap.GetInterface());
    }

    m_HeapDesc = heapDesc;

    return Result::SUCCESS;
}

Result MemoryD3D12::Create(const MemoryD3D12Desc& memoryDesc) {
    m_Heap = memoryDesc.d3d12Heap;
    m_HeapDesc = m_Heap ? m_Heap->GetDesc() : *memoryDesc.d3d12HeapDesc;

    return Result::SUCCESS;
}

void MemoryD3D12::SetPriority(ID3D12Pageable* obj) const {
    D3D12_RESIDENCY_PRIORITY priority = (D3D12_RESIDENCY_PRIORITY)m_Priority;
    if (m_Device.GetVersion() >= 1 && m_Priority)
        m_Device->SetResidencyPriority(1, &obj, &priority);
}
