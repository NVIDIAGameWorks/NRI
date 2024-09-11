// © 2021 NVIDIA Corporation

#include "SharedD3D12.h"

#include "DescriptorD3D12.h"
#include "DescriptorPoolD3D12.h"
#include "DescriptorSetD3D12.h"
#include "PipelineLayoutD3D12.h"

using namespace nri;

DescriptorSetD3D12::DescriptorSetD3D12(DescriptorPoolD3D12& desriptorPoolD3D12)
    : m_DescriptorPoolD3D12(desriptorPoolD3D12)
    , m_DynamicConstantBuffers(desriptorPoolD3D12.GetDevice().GetStdAllocator()) {
}

void DescriptorSetD3D12::Initialize(const DescriptorSetMapping* descriptorSetMapping, uint16_t dynamicConstantBufferNum) {
    m_DynamicConstantBuffers.resize(dynamicConstantBufferNum, 0);
    m_DescriptorSetMapping = descriptorSetMapping;

    for (size_t i = 0; i < m_HeapOffset.size(); i++) {
        if (m_DescriptorSetMapping->descriptorNum[i])
            m_HeapOffset[i] = m_DescriptorPoolD3D12.AllocateDescriptors((DescriptorHeapType)i, m_DescriptorSetMapping->descriptorNum[i]);
        else
            m_HeapOffset[i] = 0;
    }
}

void DescriptorSetD3D12::BuildDescriptorSetMapping(const DescriptorSetDesc& descriptorSetDesc, DescriptorSetMapping& descriptorSetMapping) {
    descriptorSetMapping.descriptorRangeMappings.resize(descriptorSetDesc.rangeNum);

    for (uint32_t i = 0; i < descriptorSetDesc.rangeNum; i++) {
        D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType = GetDescriptorHeapType(descriptorSetDesc.ranges[i].descriptorType);
        descriptorSetMapping.descriptorRangeMappings[i].descriptorHeapType = (DescriptorHeapType)descriptorHeapType;
        descriptorSetMapping.descriptorRangeMappings[i].heapOffset = descriptorSetMapping.descriptorNum[descriptorHeapType];
        descriptorSetMapping.descriptorRangeMappings[i].descriptorNum = descriptorSetDesc.ranges[i].descriptorNum;

        descriptorSetMapping.descriptorNum[descriptorHeapType] += descriptorSetDesc.ranges[i].descriptorNum;
    }
}

DescriptorPointerCPU DescriptorSetD3D12::GetPointerCPU(uint32_t rangeIndex, uint32_t rangeOffset) const {
    DescriptorHeapType descriptorHeapType = m_DescriptorSetMapping->descriptorRangeMappings[rangeIndex].descriptorHeapType;
    uint32_t heapOffset = m_HeapOffset[descriptorHeapType];
    uint32_t offset = m_DescriptorSetMapping->descriptorRangeMappings[rangeIndex].heapOffset + heapOffset + rangeOffset;
    DescriptorPointerCPU descriptorPointerCPU = m_DescriptorPoolD3D12.GetDescriptorPointerCPU(descriptorHeapType, offset);

    return descriptorPointerCPU;
}

DescriptorPointerGPU DescriptorSetD3D12::GetPointerGPU(uint32_t rangeIndex, uint32_t rangeOffset) const {
    DescriptorHeapType descriptorHeapType = m_DescriptorSetMapping->descriptorRangeMappings[rangeIndex].descriptorHeapType;
    uint32_t heapOffset = m_HeapOffset[descriptorHeapType];
    uint32_t offset = m_DescriptorSetMapping->descriptorRangeMappings[rangeIndex].heapOffset + heapOffset + rangeOffset;
    DescriptorPointerGPU descriptorPointerGPU = m_DescriptorPoolD3D12.GetDescriptorPointerGPU(descriptorHeapType, offset);

    return descriptorPointerGPU;
}

DescriptorPointerGPU DescriptorSetD3D12::GetDynamicPointerGPU(uint32_t dynamicConstantBufferIndex) const {
    return m_DynamicConstantBuffers[dynamicConstantBufferIndex];
}

//================================================================================================================
// NRI
//================================================================================================================

inline void DescriptorSetD3D12::UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    for (uint32_t i = 0; i < rangeNum; i++) {
        const DescriptorRangeMapping& rangeMapping = m_DescriptorSetMapping->descriptorRangeMappings[rangeOffset + i];
        uint32_t heapOffset = m_HeapOffset[rangeMapping.descriptorHeapType];
        uint32_t baseOffset = rangeMapping.heapOffset + heapOffset + rangeUpdateDescs[i].baseDescriptor;

        for (uint32_t j = 0; j < rangeUpdateDescs[i].descriptorNum; j++) {
            DescriptorPointerCPU dstPointer = m_DescriptorPoolD3D12.GetDescriptorPointerCPU(rangeMapping.descriptorHeapType, baseOffset + j);
            DescriptorPointerCPU srcPointer = ((DescriptorD3D12*)rangeUpdateDescs[i].descriptors[j])->GetPointerCPU();
            D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType = (D3D12_DESCRIPTOR_HEAP_TYPE)rangeMapping.descriptorHeapType;

            m_DescriptorPoolD3D12.GetDevice()->CopyDescriptorsSimple(1, {dstPointer}, {srcPointer}, descriptorHeapType);
        }
    }
}

inline void DescriptorSetD3D12::UpdateDynamicConstantBuffers(uint32_t baseBuffer, uint32_t bufferNum, const Descriptor* const* descriptors) {
    for (uint32_t i = 0; i < bufferNum; i++)
        m_DynamicConstantBuffers[baseBuffer + i] = ((DescriptorD3D12*)descriptors[i])->GetBufferLocation();
}

inline void DescriptorSetD3D12::Copy(const DescriptorSetCopyDesc& descriptorSetCopyDesc) {
    const DescriptorSetD3D12* srcDescriptorSet = (DescriptorSetD3D12*)descriptorSetCopyDesc.srcDescriptorSet;

    for (uint32_t i = 0; i < descriptorSetCopyDesc.rangeNum; i++) {
        DescriptorPointerCPU dstPointer = GetPointerCPU(descriptorSetCopyDesc.dstBaseRange + i, 0);
        DescriptorPointerCPU srcPointer = srcDescriptorSet->GetPointerCPU(descriptorSetCopyDesc.srcBaseRange + i, 0);

        uint32_t descriptorNum = m_DescriptorSetMapping->descriptorRangeMappings[i].descriptorNum;
        D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType = (D3D12_DESCRIPTOR_HEAP_TYPE)m_DescriptorSetMapping->descriptorRangeMappings[i].descriptorHeapType;

        m_DescriptorPoolD3D12.GetDevice()->CopyDescriptorsSimple(descriptorNum, {dstPointer}, {srcPointer}, descriptorHeapType);
    }

    for (uint32_t i = 0; i < descriptorSetCopyDesc.dynamicConstantBufferNum; i++) {
        DescriptorPointerGPU descriptorPointerGPU = srcDescriptorSet->GetDynamicPointerGPU(descriptorSetCopyDesc.srcBaseDynamicConstantBuffer + i);
        m_DynamicConstantBuffers[descriptorSetCopyDesc.dstBaseDynamicConstantBuffer + i] = descriptorPointerGPU;
    }
}

#include "DescriptorSetD3D12.hpp"
