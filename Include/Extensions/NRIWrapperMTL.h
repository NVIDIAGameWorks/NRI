// © 2021 NVIDIA Corporation

#pragma once

#include "NRIMacro.h"
#include "NRIDeviceCreation.h"


NriNamespaceBegin


typedef void* MTLHeap;
typedef void* MTLDeviceHandle; // id<MTLDevice>
typedef void* MTLBufferHandle; // id<MTLHeap>
typedef void* MTLTextureHandle;

NriStruct(DeviceCreationMTLDesc)
{
    bool enableNRIValidation;
    MTLDeviceHandle MtlDevice;
};

NriStruct(CommandBufferMTLDesc)
{

};

NriStruct(BufferMTLDesc)
{
  MTLBufferHandle buffer;
  void* mappedMemory;
  //MTLResourceOptions options; 
};
 
NriStruct(TextureMTLDesc)
{
  //MTLTextureHandle mtlTexture;
  //MTLTextureDescriptor* descriptor; 
};

NriStruct(MemoryMTLDesc)
{
    uint64_t size;
   // MTLStorageMode storage;
  
  //MTLResourceOptions options; 
};

NRI_API Nri(Result) NRI_CALL nriCreateDeviceFromMtlDevice(const NriRef(DeviceCreationMTLDesc) deviceDesc, NriRef(Device*) device);

NriNamespaceEnd

