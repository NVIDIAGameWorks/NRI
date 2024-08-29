// © 2021 NVIDIA Corporation

#pragma once

#include "NRIDeviceCreation.h"

NRI_NAMESPACE_BEGIN

NRI_STRUCT(DeviceCreationMTLDesc)
{
    bool enableNRIValidation;
    id<MTLDevice>     MtlDevice;
}

NRI_STRUCT(CommandBufferMTLDesc)
{

}


NRI_API NRI_NAME(Result) NRI_CALL nriCreateDeviceFromMtlDevice(const NRI_NAME_REF(DeviceCreationMTLDesc) deviceDesc, NRI_NAME_REF(Device*) device);

NRI_NAMESPACE_END

