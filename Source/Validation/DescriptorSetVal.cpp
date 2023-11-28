/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "SharedExternal.h"
#include "SharedVal.h"
#include "DescriptorSetVal.h"

#include "DescriptorVal.h"

using namespace nri;

void DescriptorSetVal::SetDebugName(const char* name)
{
    m_Name = name;
    GetCoreInterface().SetDescriptorSetDebugName(GetImpl(), name);
}

void DescriptorSetVal::UpdateDescriptorRanges(uint32_t nodeMask, uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs)
{
    if (rangeNum == 0)
        return;

    RETURN_ON_FAILURE(&m_Device, rangeUpdateDescs != nullptr, ReturnVoid(),
        "Can't update descriptor ranges: 'rangeUpdateDescs' is invalid.");

    RETURN_ON_FAILURE(&m_Device, rangeOffset < GetDesc().rangeNum, ReturnVoid(),
        "Can't update descriptor ranges: 'rangeOffset' is out of bounds. (rangeOffset=%u, rangeNum=%u)", rangeOffset, GetDesc().rangeNum);

    RETURN_ON_FAILURE(&m_Device, rangeOffset + rangeNum <= GetDesc().rangeNum, ReturnVoid(),
        "Can't update descriptor ranges: 'rangeOffset' + 'rangeNum' is greater than the number of ranges. (rangeOffset=%u, rangeNum=%u, rangeNum=%u)",
        rangeOffset, rangeNum, GetDesc().rangeNum);

    DescriptorRangeUpdateDesc* rangeUpdateDescsImpl = STACK_ALLOC(DescriptorRangeUpdateDesc, rangeNum);
    for (uint32_t i = 0; i < rangeNum; i++)
    {
        const DescriptorRangeUpdateDesc& updateDesc = rangeUpdateDescs[i];
        const DescriptorRangeDesc& rangeDesc = GetDesc().ranges[rangeOffset + i];

        RETURN_ON_FAILURE(&m_Device, updateDesc.descriptorNum != 0, ReturnVoid(),
            "Can't update descriptor ranges: 'rangeUpdateDescs[%u].descriptorNum' is zero.", i);

        RETURN_ON_FAILURE(&m_Device, updateDesc.offsetInRange < rangeDesc.descriptorNum, ReturnVoid(),
            "Can't update descriptor ranges: 'rangeUpdateDescs[%u].offsetInRange' is greater than the number of descriptors. "
            "(offsetInRange=%u, rangeDescriptorNum=%u, descriptorType=%s)",
            i, updateDesc.offsetInRange, rangeDesc.descriptorNum, GetDescriptorTypeName(rangeDesc.descriptorType));

        RETURN_ON_FAILURE(&m_Device, updateDesc.offsetInRange + updateDesc.descriptorNum <= rangeDesc.descriptorNum, ReturnVoid(),
            "Can't update descriptor ranges: 'rangeUpdateDescs[%u].offsetInRange' + 'rangeUpdateDescs[%u].descriptorNum' is greater than the number of descriptors. "
            "(offsetInRange=%u, descriptorNum=%u, rangeDescriptorNum=%u, descriptorType=%s)",
            i, i, updateDesc.offsetInRange, updateDesc.descriptorNum, rangeDesc.descriptorNum, GetDescriptorTypeName(rangeDesc.descriptorType));

        RETURN_ON_FAILURE(&m_Device, updateDesc.descriptors != nullptr, ReturnVoid(),
            "Can't update descriptor ranges: 'rangeUpdateDescs[%u].descriptors' is invalid.", i);

        DescriptorRangeUpdateDesc& dstDesc = rangeUpdateDescsImpl[i];

        dstDesc = updateDesc;
        dstDesc.descriptors = STACK_ALLOC(Descriptor*, updateDesc.descriptorNum);
        Descriptor** descriptors = (Descriptor**)dstDesc.descriptors;

        for (uint32_t j = 0; j < updateDesc.descriptorNum; j++)
        {
            RETURN_ON_FAILURE(&m_Device, updateDesc.descriptors[j] != nullptr, ReturnVoid(),
                "Can't update descriptor ranges: 'rangeUpdateDescs[%u].descriptors[%u]' is NULL.", i, j);

            descriptors[j] = NRI_GET_IMPL_PTR(Descriptor, updateDesc.descriptors[j]);
        }
    }

    GetCoreInterface().UpdateDescriptorRanges(GetImpl(), nodeMask, rangeOffset, rangeNum, rangeUpdateDescsImpl);
}

void DescriptorSetVal::UpdateDynamicConstantBuffers(uint32_t nodeMask, uint32_t baseBuffer, uint32_t bufferNum, const Descriptor* const* descriptors)
{
    if (bufferNum == 0)
        return;

    RETURN_ON_FAILURE(&m_Device, baseBuffer < GetDesc().dynamicConstantBufferNum, ReturnVoid(),
        "Can't update dynamic constant buffers: 'baseBuffer' is invalid. (baseBuffer=%u, dynamicConstantBufferNum=%u)",
        baseBuffer, GetDesc().dynamicConstantBufferNum);

    RETURN_ON_FAILURE(&m_Device, baseBuffer + bufferNum <= GetDesc().dynamicConstantBufferNum, ReturnVoid(),
        "Can't update dynamic constant buffers: 'baseBuffer' + 'bufferNum' is greater than the number of buffers. "
        "(baseBuffer=%u, bufferNum=%u, dynamicConstantBufferNum=%u)",
        baseBuffer, bufferNum, GetDesc().dynamicConstantBufferNum);

    RETURN_ON_FAILURE(&m_Device, descriptors != nullptr, ReturnVoid(),
        "Can't update dynamic constant buffers: 'descriptors' is invalid.");

    Descriptor** descriptorsImpl = STACK_ALLOC(Descriptor*, bufferNum);
    for (uint32_t i = 0; i < bufferNum; i++)
    {
        RETURN_ON_FAILURE(&m_Device, descriptors[i] != nullptr, ReturnVoid(),
            "Can't update dynamic constant buffers: 'descriptors[%u]' is invalid.", i);

        descriptorsImpl[i] = NRI_GET_IMPL_PTR(Descriptor, descriptors[i]);
    }

    GetCoreInterface().UpdateDynamicConstantBuffers(GetImpl(), nodeMask, baseBuffer, bufferNum, descriptorsImpl);
}

void DescriptorSetVal::Copy(const DescriptorSetCopyDesc& descriptorSetCopyDesc)
{
    RETURN_ON_FAILURE(&m_Device, descriptorSetCopyDesc.srcDescriptorSet != nullptr, ReturnVoid(),
        "Can't copy descriptor set: 'descriptorSetCopyDesc.srcDescriptorSet' is invalid.");

    DescriptorSetVal& srcDescriptorSetVal = *(DescriptorSetVal*)descriptorSetCopyDesc.srcDescriptorSet;
    const DescriptorSetDesc& srcDesc = srcDescriptorSetVal.GetDesc();

    RETURN_ON_FAILURE(&m_Device, descriptorSetCopyDesc.baseSrcRange < srcDesc.rangeNum, ReturnVoid(),
        "Can't copy descriptor set: 'descriptorSetCopyDesc.baseSrcRange' is invalid.");

    bool srcRangeValid = descriptorSetCopyDesc.baseSrcRange + descriptorSetCopyDesc.rangeNum < srcDesc.rangeNum;

    RETURN_ON_FAILURE(&m_Device, srcRangeValid, ReturnVoid(),
        "Can't copy descriptor set: 'descriptorSetCopyDesc.rangeNum' is invalid.");

    RETURN_ON_FAILURE(&m_Device, descriptorSetCopyDesc.baseDstRange < GetDesc().rangeNum, ReturnVoid(),
        "Can't copy descriptor set: 'descriptorSetCopyDesc.baseDstRange' is invalid.");

    bool dstRangeValid = descriptorSetCopyDesc.baseDstRange + descriptorSetCopyDesc.rangeNum < GetDesc().rangeNum;

    RETURN_ON_FAILURE(&m_Device, dstRangeValid, ReturnVoid(),
        "Can't copy descriptor set: 'descriptorSetCopyDesc.rangeNum' is invalid.");

    const bool srcOffsetValid = descriptorSetCopyDesc.baseSrcDynamicConstantBuffer < srcDesc.dynamicConstantBufferNum;

    RETURN_ON_FAILURE(&m_Device, srcOffsetValid, ReturnVoid(),
        "Can't copy descriptor set: 'descriptorSetCopyDesc.baseSrcDynamicConstantBuffer' is invalid.");

    srcRangeValid = descriptorSetCopyDesc.baseSrcDynamicConstantBuffer +
        descriptorSetCopyDesc.dynamicConstantBufferNum < srcDesc.dynamicConstantBufferNum;

    RETURN_ON_FAILURE(&m_Device, srcRangeValid, ReturnVoid(),
        "Can't copy descriptor set: source range of dynamic constant buffers is invalid.");

    const bool dstOffsetValid = descriptorSetCopyDesc.baseDstDynamicConstantBuffer < GetDesc().dynamicConstantBufferNum;

    RETURN_ON_FAILURE(&m_Device, dstOffsetValid, ReturnVoid(),
        "Can't copy descriptor set: 'descriptorSetCopyDesc.baseDstDynamicConstantBuffer' is invalid.");

    dstRangeValid = descriptorSetCopyDesc.baseDstDynamicConstantBuffer +
        descriptorSetCopyDesc.dynamicConstantBufferNum < GetDesc().dynamicConstantBufferNum;

    RETURN_ON_FAILURE(&m_Device, dstRangeValid, ReturnVoid(),
        "Can't copy descriptor set: destination range of dynamic constant buffers is invalid.");

    auto descriptorSetCopyDescImpl = descriptorSetCopyDesc;
    descriptorSetCopyDescImpl.srcDescriptorSet = NRI_GET_IMPL_PTR(DescriptorSet, descriptorSetCopyDesc.srcDescriptorSet);

    GetCoreInterface().CopyDescriptorSet(GetImpl(), descriptorSetCopyDescImpl);
}

#include "DescriptorSetVal.hpp"
