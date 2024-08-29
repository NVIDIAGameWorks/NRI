Declare_PartiallyFillFunctionTable_Functions(MTL);

#pragma region[  Core  ]

static const DeviceDesc& NRI_CALL GetDeviceDesc(const Device& device) {
    return ((const DeviceMetal&)device).GetDesc();
}

#pragma endregion


Define_Core_Device_PartiallyFillFunctionTable(MTL);
Define_Helper_Device_PartiallyFillFunctionTable(MTL);
Define_RayTracing_Device_PartiallyFillFunctionTable(MTL);
Define_Streamer_Device_PartiallyFillFunctionTable(MTL);
Define_SwapChain_Device_PartiallyFillFunctionTable(MTL);
Define_ResourceAllocator_Device_PartiallyFillFunctionTable(MTL);
