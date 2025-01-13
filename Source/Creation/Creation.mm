#import <MetalKit/MetalKit.h>

#include "SharedExternal.h"

using namespace nri;

// referenced from Molten VK
static uint32_t GetEntryProperty(io_registry_entry_t entry, CFStringRef propertyName) {
    
    uint32_t value = 0;
    
    CFTypeRef cfProp = IORegistryEntrySearchCFProperty(entry,
                                                       kIOServicePlane,
                                                       propertyName,
                                                       kCFAllocatorDefault,
                                                       kIORegistryIterateRecursively |
                                                       kIORegistryIterateParents);
    if (cfProp) {
        const uint32_t* pValue = reinterpret_cast<const uint32_t*>(CFDataGetBytePtr((CFDataRef)cfProp));
        if (pValue) { value = *pValue; }
        CFRelease(cfProp);
    }
    
    return value;
}


static int SortAdaptersByDedicatedVideoMemorySize(const void* pa, const void* pb) {
    AdapterDesc* a = (AdapterDesc*)pa;
    AdapterDesc* b = (AdapterDesc*)pb;

    if (a->videoMemorySize > b->videoMemorySize)
        return -1;

    if (a->videoMemorySize < b->videoMemorySize)
        return 1;

    return 0;
}

NRI_API Result NRI_CALL nriEnumerateAdapters(AdapterDesc* adapterDescs, uint32_t& adapterDescNum) {
    
    NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
    if(!adapterDescs) {
        adapterDescNum = (uint32_t)devices.count;
        return Result::SUCCESS;
    }
    
    AdapterDesc* adapterDescsSorted = (AdapterDesc*)alloca(sizeof(AdapterDesc) * devices.count);
    for(size_t i = 0; i < devices.count; i++) {
        NSString* name = [devices[i] name];
        [name getCString:adapterDescsSorted[i].name maxLength: sizeof(adapterDescs[i].name) - 1 encoding: NSASCIIStringEncoding];
        const uint64_t regID = [devices[i] registryID];
        adapterDescsSorted[i].luid = regID;
        if (regID)
        {
            io_registry_entry_t entry = IOServiceGetMatchingService(MACH_PORT_NULL, IORegistryEntryIDMatching(regID));
            if (entry)
            {
                // That returned the IOGraphicsAccelerator nub. Its parent, then, is the actual PCI device.
                io_registry_entry_t deviceEntry;
                if (IORegistryEntryGetParentEntry(entry, kIOServicePlane, &deviceEntry) == kIOReturnSuccess)
                {
                    adapterDescsSorted[i].vendor = GetVendorFromID(GetEntryProperty(deviceEntry, CFSTR("vendor-id"))) ;
                    adapterDescsSorted[i].deviceId = GetEntryProperty(deviceEntry, CFSTR("device-id"));
                }
            }
        } else {
            adapterDescsSorted[i].vendor = nri::Vendor::APPLE;
        }
        adapterDescsSorted[i].videoMemorySize = [devices[i] recommendedMaxWorkingSetSize];
        NSProcessInfo *pinfo = [NSProcessInfo processInfo];
        adapterDescsSorted[i].systemMemorySize = [pinfo physicalMemory];
    }
    
    // Sort by video memory size
    qsort(adapterDescsSorted, devices.count, sizeof(adapterDescsSorted[0]), SortAdaptersByDedicatedVideoMemorySize);
    for(size_t i = 0; i < MIN(devices.count, adapterDescNum); i++) {
        adapterDescs[i] = adapterDescsSorted[i];
    }
    adapterDescNum = (uint32_t)MIN(devices.count, adapterDescNum);
    return Result::SUCCESS;
}
