// © 2021 NVIDIA Corporation

#include "SharedExternal.h"
#include "SharedVal.h"

#include "DescriptorSetVal.h"
#include "DescriptorVal.h"

using namespace nri;

void DescriptorSetVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetDescriptorSetDebugName(*GetImpl(), name);
}

void DescriptorSetVal::UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    if (rangeNum == 0)
        return;

    RETURN_ON_FAILURE(&m_Device, rangeUpdateDescs != nullptr, ReturnVoid(), "UpdateDescriptorRanges: 'rangeUpdateDescs' is NULL");

    RETURN_ON_FAILURE(&m_Device, rangeOffset < GetDesc().rangeNum, ReturnVoid(), "UpdateDescriptorRanges: 'rangeOffset' is out of bounds. (rangeOffset=%u, rangeNum=%u)",
        rangeOffset, GetDesc().rangeNum);

    RETURN_ON_FAILURE(&m_Device, rangeOffset + rangeNum <= GetDesc().rangeNum, ReturnVoid(),
        "UpdateDescriptorRanges: 'rangeOffset' + 'rangeNum' is greater than the number of ranges. (rangeOffset=%u, rangeNum=%u, rangeNum=%u)", rangeOffset, rangeNum,
        GetDesc().rangeNum);

    DescriptorRangeUpdateDesc* rangeUpdateDescsImpl = StackAlloc(DescriptorRangeUpdateDesc, rangeNum);
    for (uint32_t i = 0; i < rangeNum; i++) {
        const DescriptorRangeUpdateDesc& updateDesc = rangeUpdateDescs[i];
        const DescriptorRangeDesc& rangeDesc = GetDesc().ranges[rangeOffset + i];

        RETURN_ON_FAILURE(&m_Device, updateDesc.descriptorNum != 0, ReturnVoid(), "UpdateDescriptorRanges: 'rangeUpdateDescs[%u].descriptorNum' is 0", i);
        RETURN_ON_FAILURE(&m_Device, updateDesc.descriptors != nullptr, ReturnVoid(), "UpdateDescriptorRanges: 'rangeUpdateDescs[%u].descriptors' is NULL", i);

        RETURN_ON_FAILURE(&m_Device, updateDesc.offsetInRange < rangeDesc.descriptorNum, ReturnVoid(),
            "UpdateDescriptorRanges: 'rangeUpdateDescs[%u].offsetInRange' is greater than the number of descriptors (offsetInRange=%u, rangeDescriptorNum=%u, descriptorType=%s)",
            i, updateDesc.offsetInRange, rangeDesc.descriptorNum, GetDescriptorTypeName(rangeDesc.descriptorType));

        RETURN_ON_FAILURE(&m_Device, updateDesc.offsetInRange + updateDesc.descriptorNum <= rangeDesc.descriptorNum, ReturnVoid(),
            "UpdateDescriptorRanges: 'rangeUpdateDescs[%u].offsetInRange' + 'rangeUpdateDescs[%u].descriptorNum' is greater than the number of descriptors (offsetInRange=%u, "
            "descriptorNum=%u, rangeDescriptorNum=%u, descriptorType=%s)",
            i, i, updateDesc.offsetInRange, updateDesc.descriptorNum, rangeDesc.descriptorNum, GetDescriptorTypeName(rangeDesc.descriptorType));

        DescriptorRangeUpdateDesc& dstDesc = rangeUpdateDescsImpl[i];

        dstDesc = updateDesc;
        dstDesc.descriptors = StackAlloc(Descriptor*, updateDesc.descriptorNum);
        Descriptor** descriptors = (Descriptor**)dstDesc.descriptors;

        for (uint32_t j = 0; j < updateDesc.descriptorNum; j++) {
            RETURN_ON_FAILURE(&m_Device, updateDesc.descriptors[j] != nullptr, ReturnVoid(), "UpdateDescriptorRanges: 'rangeUpdateDescs[%u].descriptors[%u]' is NULL", i, j);

            descriptors[j] = NRI_GET_IMPL(Descriptor, updateDesc.descriptors[j]);
        }
    }

    GetCoreInterface().UpdateDescriptorRanges(*GetImpl(), rangeOffset, rangeNum, rangeUpdateDescsImpl);
}

void DescriptorSetVal::UpdateDynamicConstantBuffers(uint32_t baseBuffer, uint32_t bufferNum, const Descriptor* const* descriptors) {
    if (bufferNum == 0)
        return;

    RETURN_ON_FAILURE(&m_Device, baseBuffer < GetDesc().dynamicConstantBufferNum, ReturnVoid(),
        "UpdateDynamicConstantBuffers: 'baseBuffer' is invalid. (baseBuffer=%u, dynamicConstantBufferNum=%u)", baseBuffer, GetDesc().dynamicConstantBufferNum);

    RETURN_ON_FAILURE(&m_Device, baseBuffer + bufferNum <= GetDesc().dynamicConstantBufferNum, ReturnVoid(),
        "UpdateDynamicConstantBuffers: 'baseBuffer' + 'bufferNum' is greater than the number of buffers (baseBuffer=%u, bufferNum=%u, dynamicConstantBufferNum=%u)", baseBuffer,
        bufferNum, GetDesc().dynamicConstantBufferNum);

    RETURN_ON_FAILURE(&m_Device, descriptors != nullptr, ReturnVoid(), "UpdateDynamicConstantBuffers: 'descriptors' is NULL");

    Descriptor** descriptorsImpl = StackAlloc(Descriptor*, bufferNum);
    for (uint32_t i = 0; i < bufferNum; i++) {
        RETURN_ON_FAILURE(&m_Device, descriptors[i] != nullptr, ReturnVoid(), "UpdateDynamicConstantBuffers: 'descriptors[%u]' is NULL", i);

        descriptorsImpl[i] = NRI_GET_IMPL(Descriptor, descriptors[i]);
    }

    GetCoreInterface().UpdateDynamicConstantBuffers(*GetImpl(), baseBuffer, bufferNum, descriptorsImpl);
}

void DescriptorSetVal::Copy(const DescriptorSetCopyDesc& descriptorSetCopyDesc) {
    RETURN_ON_FAILURE(&m_Device, descriptorSetCopyDesc.srcDescriptorSet != nullptr, ReturnVoid(), "CopyDescriptorSet: 'descriptorSetCopyDesc.srcDescriptorSet' is NULL");

    DescriptorSetVal& srcDescriptorSetVal = *(DescriptorSetVal*)descriptorSetCopyDesc.srcDescriptorSet;
    const DescriptorSetDesc& srcDesc = srcDescriptorSetVal.GetDesc();

    RETURN_ON_FAILURE(&m_Device, descriptorSetCopyDesc.baseSrcRange < srcDesc.rangeNum, ReturnVoid(), "CopyDescriptorSet: 'descriptorSetCopyDesc.baseSrcRange' is invalid");

    bool srcRangeValid = descriptorSetCopyDesc.baseSrcRange + descriptorSetCopyDesc.rangeNum < srcDesc.rangeNum;
    bool dstRangeValid = descriptorSetCopyDesc.baseDstRange + descriptorSetCopyDesc.rangeNum < GetDesc().rangeNum;
    bool srcOffsetValid = descriptorSetCopyDesc.baseSrcDynamicConstantBuffer < srcDesc.dynamicConstantBufferNum;
    bool srcDynamicConstantBufferValid = descriptorSetCopyDesc.baseSrcDynamicConstantBuffer + descriptorSetCopyDesc.dynamicConstantBufferNum < srcDesc.dynamicConstantBufferNum;
    bool dstOffsetValid = descriptorSetCopyDesc.baseDstDynamicConstantBuffer < GetDesc().dynamicConstantBufferNum;
    bool dstDynamicConstantBufferValid = descriptorSetCopyDesc.baseDstDynamicConstantBuffer + descriptorSetCopyDesc.dynamicConstantBufferNum < GetDesc().dynamicConstantBufferNum;

    RETURN_ON_FAILURE(&m_Device, srcRangeValid, ReturnVoid(), "CopyDescriptorSet: 'descriptorSetCopyDesc.rangeNum' is invalid");
    RETURN_ON_FAILURE(&m_Device, descriptorSetCopyDesc.baseDstRange < GetDesc().rangeNum, ReturnVoid(), "CopyDescriptorSet: 'descriptorSetCopyDesc.baseDstRange' is invalid");
    RETURN_ON_FAILURE(&m_Device, dstRangeValid, ReturnVoid(), "CopyDescriptorSet: 'descriptorSetCopyDesc.rangeNum' is invalid");
    RETURN_ON_FAILURE(&m_Device, srcOffsetValid, ReturnVoid(), "CopyDescriptorSet: 'descriptorSetCopyDesc.baseSrcDynamicConstantBuffer' is invalid");
    RETURN_ON_FAILURE(&m_Device, srcDynamicConstantBufferValid, ReturnVoid(), "CopyDescriptorSet: source range of dynamic constant buffers is invalid");
    RETURN_ON_FAILURE(&m_Device, dstOffsetValid, ReturnVoid(), "CopyDescriptorSet: 'descriptorSetCopyDesc.baseDstDynamicConstantBuffer' is invalid");
    RETURN_ON_FAILURE(&m_Device, dstDynamicConstantBufferValid, ReturnVoid(), "CopyDescriptorSet: destination range of dynamic constant buffers is invalid");

    auto descriptorSetCopyDescImpl = descriptorSetCopyDesc;
    descriptorSetCopyDescImpl.srcDescriptorSet = NRI_GET_IMPL(DescriptorSet, descriptorSetCopyDesc.srcDescriptorSet);

    GetCoreInterface().CopyDescriptorSet(*GetImpl(), descriptorSetCopyDescImpl);
}

#include "DescriptorSetVal.hpp"
