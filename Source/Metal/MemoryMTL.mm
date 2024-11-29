// © 2021 NVIDIA Corporation
#include "SharedMTL.h"

#include "MemoryMTL.h"

using namespace nri;

MemoryMTL::~MemoryMTL() {
    //if (m_OwnsNativeObjects) {
    //    const auto& vk = m_Device.GetDispatchTable();
    //    vk.FreeMemory(m_Device, m_Handle, m_Device.GetAllocationCallbacks());
    //}
}

Result MemoryMTL::Create(const AllocateMemoryDesc& allocateMemoryDesc) {
    MTLHeapDescriptor* heapDescriptor = [[MTLHeapDescriptor alloc] init];
    MemoryTypeInfo typeInfo;
    typeInfo.value = allocateMemoryDesc.type;
    [heapDescriptor setCpuCacheMode: (MTLCPUCacheMode)typeInfo.cacheMode];
    [heapDescriptor setStorageMode: (MTLStorageMode)typeInfo.storageMode];
    [heapDescriptor setResourceOptions: (MTLResourceOptions)typeInfo.options];
    [heapDescriptor setSize: allocateMemoryDesc.size];
    m_Handle = [m_Device newHeapWithDescriptor: heapDescriptor];
    m_Type = allocateMemoryDesc.type;
    m_Priority = allocateMemoryDesc.priority;
}

void MemoryMTL::SetDebugName(const char* name) {
    NSString* str = [NSString stringWithUTF8String:name];
    [m_Handle setLabel: str];
}


//Result MemoryMTL::Create(const MemoryMTLDesc& memoryDesc) {
//    MTLHeapDescriptor* heapDescriptor = [[MTLHeapDescriptor alloc] init];
//    
//    [heapDescriptor setSize:memoryDesc.size];
//    
//   // [heapDescriptor setStorageMode:memoryDesc.storage];
//
//   // if (!memoryDesc.vkDeviceMemory)
//   //     return Result::INVALID_ARGUMENT;
//
//   // MemoryTypeInfo memoryTypeInfo = {};
//
//    //bool found = m_Device.GetMemoryTypeByIndex(memoryDesc.memoryTypeIndex, memoryTypeInfo);
//    //RETURN_ON_FAILURE(&m_Device, found, Result::INVALID_ARGUMENT, "Can't find memory by index");
//
////    m_OwnsNativeObjects = false;
//   // m_Buffer = memoryDesc.buffer;
////    m_MappedMemory = (uint8_t*)memoryDesc.mappedMemory;
//   // m_Type = memoryDesc.options;
//
//   // const auto& mtl = m_Device.GetDispatchTable();
//    //if (!m_MappedMemory && IsHostVisibleMemory(memoryTypeInfo.location)) {
//    //    VkResult result = mtl.MapMemory(m_Device, m_Handle, 0, memoryDesc.size, 0, (void**)&m_MappedMemory);
//    //    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkMapMemory returned %d", (int32_t)result);
//    //}
//
//    return Result::SUCCESS;
//}

