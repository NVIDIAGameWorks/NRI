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

//ResultOrError<MTLGPUFamily> GetMTLGPUFamily(id<MTLDevice> device) {
//    // https://developer.apple.com/documentation/metal/mtldevice/detecting_gpu_features_and_metal_software_versions?language=objc
//
//    if (@available(macOS 10.15, iOS 10.13, *)) {
//        if ([device supportsFamily:MTLGPUFamilyApple7]) {
//            return MTLGPUFamily::Apple7;
//        }
//        if ([device supportsFamily:MTLGPUFamilyApple6]) {
//            return MTLGPUFamily::Apple6;
//        }
//        if ([device supportsFamily:MTLGPUFamilyApple5]) {
//            return MTLGPUFamily::Apple5;
//        }
//        if ([device supportsFamily:MTLGPUFamilyApple4]) {
//            return MTLGPUFamily::Apple4;
//        }
//        if ([device supportsFamily:MTLGPUFamilyApple3]) {
//            return MTLGPUFamily::Apple3;
//        }
//        if ([device supportsFamily:MTLGPUFamilyApple2]) {
//            return MTLGPUFamily::Apple2;
//        }
//        if ([device supportsFamily:MTLGPUFamilyApple1]) {
//            return MTLGPUFamily::Apple1;
//        }
//
//        // This family is no longer supported in the macOS 10.15 SDK but still exists so
//        // default to it.
//        return MTLGPUFamily::Mac1;
//    }
//
//    return DAWN_INTERNAL_ERROR("Unsupported Metal device");
//}
//
//}  // anonymous namespace

FormatSupportBits DeviceMTL::GetFormatSupport(const Device& device, Format format) {
  int currentFamily = HIGHEST_GPU_FAMILY;
  for (; currentFamily >= (int)MTLGPUFamilyApple1; currentFamily--) {
    if ([m_device supportsFamily:(MTLGPUFamily)currentFamily]) {
      highestAppleFamily = (MTLGPUFamily)currentFamily;
      break;
    }
  }
}

static inline bool FindMTLGpuFamily(id<MTLDevice> device, MTLGPUFamily* family) {
    //https://developer.apple.com/documentation/metal/mtldevice/detecting_gpu_features_and_metal_software_versions?language=objc 
    if (@available(macOS 10.15, iOS 10.13, *)) {
        if ([device supportsFamily:MTLGPUFamilyApple7]) {
            return MTLGPUFamily::Apple7;
        }
        if ([device supportsFamily:MTLGPUFamilyApple6]) {
            return MTLGPUFamily::Apple6;
        }
        if ([device supportsFamily:MTLGPUFamilyApple5]) {
            return MTLGPUFamily::Apple5;
        }
        if ([device supportsFamily:MTLGPUFamilyApple4]) {
            return MTLGPUFamily::Apple4;
        }
        if ([device supportsFamily:MTLGPUFamilyApple3]) {
            return MTLGPUFamily::Apple3;
        }
        if ([device supportsFamily:MTLGPUFamilyApple2]) {
            return MTLGPUFamily::Apple2;
        }
        if ([device supportsFamily:MTLGPUFamilyApple1]) {
            return MTLGPUFamily::Apple1;
        }

        // This family is no longer supported in the macOS 10.15 SDK but still exists so
        // default to it.
        //return MTLGPUFamily::Mac1;
        (*family) = MTLGPUFamily::Mac1;
        return true;
    }

    return false;
}


Result DeviceMTL::Create(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationMTLDesc& deviceCreationMTLDesc, bool isWrapper) {
    m_OwnsNativeObjects = !isWrapper;

    if(isWrapper) {
        m_Device = deviceCreationMTLDesc.MtlDevice;
    }
  
    strncpy(m_Desc.adapterDesc.name, [m_Device.name UTF8String], sizeof(m_Desc.adapterDesc.name));
    // No vendor id, device id for Apple GPUs
    if (strstr(m_Desc.adapterDesc.name, "Apple")) {
        m_Desc.adapterDesc.vendor = nri::Vendor::APPLE;
    } 

    const uint64_t regID = [m_Device registryID];
    if (regID)
    {
        IORef<io_registry_entry_t> entry =AcquireIORef(IOServiceGetMatchingService(kIOMasterPortDefault, IORegistryEntryIDMatching(regID)));
        if (entry)
        {
            // That returned the IOGraphicsAccelerator nub. Its parent, then, is the actual PCI device.
        IORef<io_registry_entry_t> deviceEntry;
            if (IORegistryEntryGetParentEntry(entry, kIOServicePlane, &deviceEntry) == kIOReturnSuccess)
            {
                m_Desc.adapterDesc.vendor = GetVendorFromID(GetEntryProperty(deviceEntry, CFSTR("vendor-id")));
                m_Desc.adapterDesc.deviceId = GetEntryProperty(deviceEntry, CFSTR("device-id"));
            }
        }
    }

    m_Desc.adapterDesc.luid = 0;
    m_Desc.adapterDesc.videoMemorySize = 0;
    m_Desc.adapterDesc.systemMemorySize = 0;
    return Result::SUCCESS;

}
