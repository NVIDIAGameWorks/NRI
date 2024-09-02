#include "SharedMTL.h"

#include "CommandQueueMTL.h"

#include "DeviceMTL.h"

using namespace nri;

static bool FindMTLGpuFamily(id<MTLDevice> device,
                                    MTLGPUFamily *family) {
    // https://developer.apple.com/documentation/metal/mtldevice/detecting_gpu_features_and_metal_software_versions?language=objc
    if (@available(macOS 10.15, iOS 10.13, *)) {
        if ([device supportsFamily:MTLGPUFamilyApple7]) {
            (*family) = MTLGPUFamily::MTLGPUFamilyApple7;
            return true;
        }
        if ([device supportsFamily:MTLGPUFamilyApple6]) {
            (*family) = MTLGPUFamily::MTLGPUFamilyApple6;
            return true;
        }
        if ([device supportsFamily:MTLGPUFamilyApple5]) {
            (*family) = MTLGPUFamily::MTLGPUFamilyApple5;
            return true;
        }
        if ([device supportsFamily:MTLGPUFamilyApple4]) {
            (*family) = MTLGPUFamily::MTLGPUFamilyApple4;
            return true;
        }
        if ([device supportsFamily:MTLGPUFamilyApple3]) {
            (*family) = MTLGPUFamily::MTLGPUFamilyApple3;
            return true;
        }
        if ([device supportsFamily:MTLGPUFamilyApple2]) {
            (*family) = MTLGPUFamily::MTLGPUFamilyApple2;
            return true;
        }
        if ([device supportsFamily:MTLGPUFamilyApple1]) {
            (*family) = MTLGPUFamily::MTLGPUFamilyApple1;
            return true;
        }
        
        // This family is no longer supported in the macOS 10.15 SDK but still
        // exists so default to it.
        // return MTLGPUFamily::Mac1;
        (*family) = MTLGPUFamily::MTLGPUFamilyMac1;
        return true;
    }
    return false;
}



DeviceMTL::DeviceMTL(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator) : DeviceBase(callbacks, stdAllocator) {
    m_Desc.graphicsAPI = GraphicsAPI::VK;
    m_Desc.nriVersionMajor = NRI_VERSION_MAJOR;
    m_Desc.nriVersionMinor = NRI_VERSION_MINOR;

}


void DeviceMTL::Destruct() {
    Destroy(GetStdAllocator(), this);
}

DeviceMTL::~DeviceMTL() {

}


//FormatSupportBits DeviceMTL::GetFormatSupport(const Device& device, Format format) {
//  int currentFamily = HIGHEST_GPU_FAMILY;
//  for (; currentFamily >= (int)MTLGPUFamilyApple1; currentFamily--) {
//    if ([m_Device supportsFamily:(MTLGPUFamily)currentFamily]) {
//      highestAppleFamily = (MTLGPUFamily)currentFamily;
//      break;
//    }
//  }
//}


Result DeviceMTL::GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue) {
    ExclusiveScope lock(m_Lock);

    // Check if already created (or wrapped)
    uint32_t index = (uint32_t)commandQueueType;
    if (m_CommandQueues[index]) {
        commandQueue = (CommandQueue*)m_CommandQueues[index];
        return Result::SUCCESS;
    }

    // Check if supported
    //uint32_t queueFamilyIndex = m_QueueFamilyIndices[index];
    //if (queueFamilyIndex == INVALID_FAMILY_INDEX) {
    //    commandQueue = nullptr;
    //    return Result::UNSUPPORTED;
    //}

    // Create
    //VkQueue handle = VK_NULL_HANDLE;
    //m_VK.GetDeviceQueue(m_Device, queueFamilyIndex, 0, &handle);

    Result result = CreateImplementation<CommandQueueMTL>(commandQueue, commandQueueType);
    if (result == Result::SUCCESS)
        m_CommandQueues[index] = (CommandQueueMTL*)commandQueue;

    return result;
}

//void DeviceMTL::FillCreateInfo(const TextureDesc& textureDesc, MTLTextureDescriptor* info) const {
//    info.textureType = ::GetImageTypeMTL(textureDesc.type);
//    //info.pixelFormat = ::GetFormatMTL(textureDesc.format, true);
//    //info.width = pDesc->mWidth;
//    //info.height = pDesc->mHeight;
//    //info.depth = pDesc->mDepth;
//    //info.mipmapLevelCount = mipLevels;
//    //info.sampleCount = pDesc->mSampleCount;
//    //info.arrayLength = pDesc->mArraySize;
//}

Result DeviceMTL::Create(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationMTLDesc& deviceCreationMTLDesc, bool isWrapper) {
    m_OwnsNativeObjects = !isWrapper;
    
    if(isWrapper) {
        //   m_Device = deviceCreationMTLDesc.MtlDevice;
    }
    
    strncpy(m_Desc.adapterDesc.name, [m_Device.name UTF8String], sizeof(m_Desc.adapterDesc.name));
    // No vendor id, device id for Apple GPUs
    if (strstr(m_Desc.adapterDesc.name, "Apple")) {
        m_Desc.adapterDesc.vendor = nri::Vendor::APPLE;
    }
    
    const uint64_t regID = [m_Device registryID];
    if (regID)
    {
        //  IORef<io_registry_entry_t> entry =AcquireIORef(IOServiceGetMatchingService(kIOMasterPortDefault, IORegistryEntryIDMatching(regID)));
        //  if (entry)
        //  {
        // That returned the IOGraphicsAccelerator nub. Its parent, then, is the actual PCI device.
        //  IORef<io_registry_entry_t> deviceEntry;
        //       if (IORegistryEntryGetParentEntry(entry, kIOServicePlane, &deviceEntry) == kIOReturnSuccess)
        //       {
        //           m_Desc.adapterDesc.vendor = GetVendorFromID(GetEntryProperty(deviceEntry, CFSTR("vendor-id")));
        //           m_Desc.adapterDesc.deviceId = GetEntryProperty(deviceEntry, CFSTR("device-id"));
        //        }
        // }
    }
    
    MTLGPUFamily family;
    //  if(!FindMTLGpuFamily(m_Device, family)) {
    //     return Result::UNSUPPORTED;
    //}
    // https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
    //TODO: fill desc 
    switch(family) {
    //    case MTLGPUFamily::Apple1:
     //       break;
        default:
            break;
    }

    m_Desc.adapterDesc.luid = 0;
    m_Desc.adapterDesc.videoMemorySize = 0;
    m_Desc.adapterDesc.systemMemorySize = 0;
    return Result::SUCCESS;

}

