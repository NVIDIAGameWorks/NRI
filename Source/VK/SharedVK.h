/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include "SharedExternal.h"
#include "DeviceBase.h"

#include <vulkan/vulkan.h>
#undef CreateSemaphore

#include "DispatchTable.h"

struct MemoryTypeInfo
{
    uint16_t memoryTypeIndex;
    uint8_t location;
    uint8_t isDedicated : 1;
    uint8_t isHostCoherent : 1;
};
static_assert(sizeof(MemoryTypeInfo) <= sizeof(nri::MemoryType), "Unexpected structure size");

union MemoryTypeUnpack
{
    nri::MemoryType type;
    MemoryTypeInfo info;
};

template< typename HandleType, typename ImplType, typename NRIType >
constexpr HandleType GetVulkanHandle(NRIType* object, uint32_t nodeIndex)
{ return (object != nullptr) ? (*(ImplType*)object).GetHandle(nodeIndex) : HandleType(VK_NULL_HANDLE); }

constexpr bool IsHostVisibleMemory(nri::MemoryLocation location)
{ return location > nri::MemoryLocation::DEVICE; }

// TODO: mostly needed for AMD?
#if 1
    #define IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    #define IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
#else
    #define IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL VK_IMAGE_LAYOUT_GENERAL
    #define IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL VK_IMAGE_LAYOUT_GENERAL
#endif

#include "ConversionVK.h"
#include "DeviceVK.h"
