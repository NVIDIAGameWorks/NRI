#include "DeviceMetal.h"

using namespace nri;

BufferMetal::BufferMetal(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator) 
    : DeviceBase(callbacks, stdAllocator) {
    m_Desc.graphicsAPI = GraphicsAPI::VK;
    m_Desc.nriVersionMajor = NRI_VERSION_MAJOR;
    m_Desc.nriVersionMinor = NRI_VERSION_MINOR;

}
BufferMetal::~BufferMetal() {

}

Result CreateDeviceMTL(const DeviceCreationDesc& deviceCreationDesc, DeviceBase*& device) {
    
}

Result CreateDeviceMTL(const DeviceCreationMTLDesc& deviceCreationDesc, DeviceBase*& device) {

}

Result DeviceMetal::Create(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationMTLDesc& deviceCreationMTLDesc, bool isWrapper) {
  m_OwnsNativeObjects = !isWrapper;

  if(isWrapper) {
    m_Device = deviceCreationMTLDesc.MtlDevice;
  }
  
  strncpy(m_Desc.adapterDesc.name, [m_Device.name UTF8String], sizeof(m_Desc.adapterDesc.name));
  // No vendor id, device id for Apple GPUs
  if (strstr(m_Desc.adapterDesc.name, "Apple"))
  {
      m_Desc.adapterDesc.vendor = nri::Vendor::APPLE;
  } else {
      const uint64_t regID = [m_Device registryID];
      if (regID)
      {
          io_registry_entry_t entry = IOServiceGetMatchingService(kIOMasterPortDefault, IORegistryEntryIDMatching(regID));
          if (entry)
          {
              // That returned the IOGraphicsAccelerator nub. Its parent, then, is the actual PCI device.
              io_registry_entry_t parent;
              if (IORegistryEntryGetParentEntry(entry, kIOServicePlane, &parent) == kIOReturnSuccess)
              {
                  m_Desc.adapterDesc.vendor = GetVendorFromID(GetEntryProperty(parent, CFSTR("vendor-id")));
                  m_Desc.adapterDesc.deviceId = GetEntryProperty(parent, CFSTR("device-id"));
                  IOObjectRelease(parent);
              }
              IOObjectRelease(entry);
          }
      }
  }


}
