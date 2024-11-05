// © 2021 NVIDIA Corporation

#pragma once

#include "NRIDeviceCreation.h"

NriNamespaceBegin

typedef void* MTLBufferHandle; 
typedef void* MTLTextureHandle; 
typedef void* MTLDeviceHandle; 

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
  MTLBufferHandle buffer;
  void* mappedMemory;
  //MTLResourceOptions options; 
};

NRI_API Nri(Result) NRI_CALL nriCreateDeviceFromMtlDevice(const NriRef(DeviceCreationMTLDesc) deviceDesc, NriRef(Device*) device);

NriNamespaceEnd

