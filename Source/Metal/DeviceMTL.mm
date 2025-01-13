

#include "SharedMTL.h"

#include "CommandQueueMTL.h"
#include "DeviceMTL.h"
#include "BufferMTL.h"
#include "CommandQueueMTL.h"
#include "TextureMTL.h"
#include "FenceMTL.h"
#include "MemoryMTL.h"
#include "PipelineLayoutMTL.h"
#include "PipelineMTL.h"

using namespace nri;

static bool FindMTLGpuFamily(id<MTLDevice> device,
                             const MTLGPUFamily *families, size_t len,
                             MTLGPUFamily* current) {
    for(size_t i = 0; i < len; i++) {
        if( [device supportsFamily: families[i]]) {
            *current = families[i];
            return true;
        }
    }
    return false;
}

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

DeviceMTL::DeviceMTL(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator)
: DeviceBase(callbacks, stdAllocator) {
    m_Desc.graphicsAPI = GraphicsAPI::MTL;
    m_Desc.nriVersionMajor = NRI_VERSION_MAJOR;
    m_Desc.nriVersionMinor = NRI_VERSION_MINOR;
}


void DeviceMTL::Destruct() {
    for (uint32_t i = 0; i < m_CommandQueues.size(); i++)
        Destroy(GetStdAllocator(), m_CommandQueues[i]);
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


static id<MTLFunction> initShaderFromSource(id<MTLDevice> dev, NSString* src, NSString* method) {
    id<MTLFunction> mtlFunc = nil;
    NSError* err = nil;
    
    MTLCompileOptions* options = [MTLCompileOptions alloc];
    id<MTLLibrary> mtlLib = [dev newLibraryWithSource: src
                                                   options: options
                                                     error: &err];    // temp retain
    NSCAssert(err,  @"Failed to load Metal shader library %@", err );
    
    return [mtlLib newFunctionWithName: method];
}


static id<MTLRenderPipelineState> CreateClearPipeline(id<MTLDevice> device, const ClearDesc* desc, size_t numFormats) {
    NSString* clearVert = [NSString stringWithCString: "\
#include <metal_stdlib> \n\
using namespace metal; \n\
typedef struct { \n\
  float4 a_position [[attribute(0)]]; \n\
} AttributesPos; \n\
typedef struct { \n\
  float4 colors[16]; \n\
} ClearColorsIn; \n\
typedef struct { \n\
  float4 v_position [[position]]; \n\
  uint layer [[render_target_array_index]]; \n\
} VaryingsPos; \n\
vertex VaryingsPos vertClear(AttributesPos attributes [[stage_in]], constant ClearColorsIn& ccIn [[buffer(0)]]) { \n\
   VaryingsPos varyings; \n \
   varyings.v_position = float4(attributes.a_position.x, -attributes.a_position.y, ccIn.colors[4].r, 1.0); \n\
   varyings.layer = uint(attributes.a_position.w); \n\
   return varyings; \n\
}" encoding: NSASCIIStringEncoding];
    
    NSMutableString* clearFrag = [NSMutableString alloc];
    [clearFrag appendString: @"\
 #include <metal_stdlib> \n\
 using namespace metal; \n\
 typedef struct { \n\
    loat4 v_position [[position]]; \n\
 } VaryingsPos; \n\
     typedef struct { \n\
     float4 colors[16]; \n\
 } ClearColorsIn; \n\
 typedef struct { \n\
     "];
    for(uint32_t i = 0; i < numFormats; i++) {
        [clearFrag appendFormat: @"float4 color%u [[color(%u)]];", i, desc[i].colorAttachmentIndex];
    }
    [clearFrag appendString: @"\
    } ClearColorsOut; \n\
    fragment ClearColorsOut fragClear(VaryingsPos varyings [[stage_in]], constant ClearColorsIn& ccIn [[buffer(0)]]) { \n\
    ClearColorsOut ccOut; \n"];
    for(uint32_t i = 0; i < numFormats; i++) {
        [clearFrag appendFormat: @"ccOut.color%u = float4(ccIn.colors[%u]);", i, i];
    }
    [clearFrag appendString: @"\
return ccOut;\n\
}"];
    
    id<MTLFunction> vtxFunc = initShaderFromSource(device, clearVert, @"vertClear");
    id<MTLFunction> fragFunc = initShaderFromSource(device, clearFrag, @"fragClear");
    MTLRenderPipelineDescriptor* renderPipelineDesc = [MTLRenderPipelineDescriptor new];    // temp retain
    //    owner->setMetalObjectLabel(plDesc, @"ClearRenderAttachments");
    renderPipelineDesc.vertexFunction = vtxFunc;
    renderPipelineDesc.fragmentFunction = fragFunc;
    //    plDesc.sampleCount = 1;
    renderPipelineDesc.inputPrimitiveTopology = MTLPrimitiveTopologyClassTriangle;
    
    MTLVertexDescriptor* vtxDesc = renderPipelineDesc.vertexDescriptor;
    
    MTLVertexAttributeDescriptorArray* vaDescArray = vtxDesc.attributes;
    MTLVertexAttributeDescriptor* vaDesc;
    NSUInteger vtxBuffIdx = 0;
    NSUInteger vtxStride = 0;
    
    // Vertex location
    vaDesc = vaDescArray[0];
    vaDesc.format = MTLVertexFormatFloat4;
    vaDesc.bufferIndex = vtxBuffIdx;
    vaDesc.offset = vtxStride;
    vtxStride += sizeof(simd::float4);
    
    // Vertex attribute buffer.
    MTLVertexBufferLayoutDescriptorArray* vbDescArray = vtxDesc.layouts;
    MTLVertexBufferLayoutDescriptor* vbDesc = vbDescArray[vtxBuffIdx];
    vbDesc.stepFunction = MTLVertexStepFunctionPerVertex;
    vbDesc.stepRate = 1;
    vbDesc.stride = vtxStride;
    
    NSError* error = nil;
    id <MTLRenderPipelineState> rps = [device newRenderPipelineStateWithDescriptor:renderPipelineDesc error: &error];
    
    [vtxFunc release];                                                            // temp release
    [fragFunc release];                                                            // temp release
    [renderPipelineDesc release];                                                            // temp release
    
    return rps;
}

//TODO: implement this differently
id<MTLRenderPipelineState> DeviceMTL::GetClearPipeline(const ClearDesc* desc, size_t numFormats) {
    uint32_t key = 0;
    for(uint32_t i = 0; i < numFormats; i++) {
        key |= (1 << desc[i].colorAttachmentIndex);
    }
    id<MTLRenderPipelineState> rps = CreateClearPipeline(m_Device, desc, numFormats);
    m_clearPipelineState[key] = rps;
    return rps;
}


void DeviceMTL::GetMemoryDesc(const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    MemoryTypeInfo memoryTypeInfo;
    memoryTypeInfo.options = (uint32_t)DEFAULT_MEMORY_RESOURCE_OPTION_MEMORY_LOCATION[(size_t)memoryLocation];
    memoryTypeInfo.cacheMode = (uint32_t)DEFAULT_CACHE_MODE_MEMORY_LOCATION[(size_t)memoryLocation];
    memoryTypeInfo.storageMode = (uint32_t)DEFAULT_STORAGE_MODE_MEMORY_LOCATION[(size_t)memoryLocation];
    MTLTextureDescriptor* mtlTextureDesc = [[MTLTextureDescriptor alloc] init];
    MTLSizeAndAlign sizeAlign =  [m_Device heapBufferSizeAndAlignWithLength: bufferDesc.size options: (MTLResourceOptions)memoryTypeInfo.options];
    
    memoryDesc.size = sizeAlign.size;
    memoryDesc.alignment = (uint32_t)sizeAlign.align;
    memoryDesc.type = memoryTypeInfo.value;
    
}

void DeviceMTL::GetMemoryDesc(const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    MTLTextureDescriptor* mtlTextureDesc = [[MTLTextureDescriptor alloc] init];
   
    MemoryTypeInfo memoryTypeInfo;
    memoryTypeInfo.options = (uint32_t)DEFAULT_MEMORY_RESOURCE_OPTION_MEMORY_LOCATION[(size_t)memoryLocation];
    memoryTypeInfo.cacheMode = (uint32_t)DEFAULT_CACHE_MODE_MEMORY_LOCATION[(size_t)memoryLocation];
    memoryTypeInfo.storageMode = (uint32_t)DEFAULT_STORAGE_MODE_MEMORY_LOCATION[(size_t)memoryLocation];
    nri::fillMTLTextureDescriptor(textureDesc, mtlTextureDesc);
    const MTLSizeAndAlign sizeAlign = [m_Device heapTextureSizeAndAlignWithDescriptor: mtlTextureDesc];
    
    memoryDesc.size = sizeAlign.size;
    memoryDesc.alignment = (uint32_t)sizeAlign.align;
    memoryDesc.type = memoryTypeInfo.value;
}

void DeviceMTL::GetMemoryDesc(const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
//    MTLAccelerationStructureDescriptor* mtlAcceleration = [[MTLAccelerationStructureDescriptor alloc] init];
//    [mtlAcceleration set]
    
   // [m_Device heapAccelerationStructureSizeAndAlignWithSize: accelerationStructureDesc.geometryObjects]
    // TODO: need to add ray traced
}


Result DeviceMTL::GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue) {
    ExclusiveScope lock(m_Lock);

    // Check if already created (or wrapped)
    uint32_t index = (uint32_t)commandQueueType;
    if (m_CommandQueues[index]) {
        commandQueue = (CommandQueue*)m_CommandQueues[index];
        return Result::SUCCESS;
    }
    Result result = CreateImplementation<CommandQueueMTL>(commandQueue, commandQueueType);
    if (result == Result::SUCCESS)
        m_CommandQueues[index] = (CommandQueueMTL*)commandQueue;

    return result;
}

Result DeviceMTL::BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    if (!memoryBindingDescNum)
        return Result::SUCCESS;

    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        const BufferMemoryBindingDesc& memoryBindingDesc = memoryBindingDescs[i];

        BufferMTL& bufferImpl = *(BufferMTL*)memoryBindingDesc.buffer;
        MemoryMTL& memoryImpl = *(MemoryMTL*)memoryBindingDesc.memory;

        bufferImpl.FinishMemoryBinding(memoryImpl, memoryBindingDesc.offset);
    }
    
    return Result::SUCCESS;
}

Result DeviceMTL::BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    if (!memoryBindingDescNum)
        return Result::SUCCESS;

    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        const TextureMemoryBindingDesc& memoryBindingDesc = memoryBindingDescs[i];

        TextureMTL& textureImpl = *(TextureMTL*)memoryBindingDesc.texture;
        MemoryMTL& memoryImpl = *(MemoryMTL*)memoryBindingDesc.memory;

        textureImpl.FinishMemoryBinding(memoryImpl, memoryBindingDesc.offset);
    }
    
    return Result::SUCCESS;
}


Result DeviceMTL::Create(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationMTLDesc& deviceCreationMTLDesc, bool isWrapper) {
    m_OwnsNativeObjects = !isWrapper;
    if(isWrapper) {
        m_Device = *(id<MTLDevice>*)&deviceCreationMTLDesc.MtlDevice;
    } else {
        NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
        uint32_t i = 0;
        for(i = 0; i < devices.count; i++) {
            if(deviceCreationDesc.adapterDesc) {
                const uint64_t luid = [devices[i] registryID];
                if(deviceCreationDesc.adapterDesc->luid == luid) {
                    m_Device = devices[i];
                    break;
                }
            } else {
                break;
            }
        }
        RETURN_ON_FAILURE(this, i != devices.count, Result::INVALID_ARGUMENT, "Can't create a device: physical device not found");
    }
    
    strncpy(m_Desc.adapterDesc.name, [m_Device.name UTF8String], sizeof(m_Desc.adapterDesc.name));
    // No vendor id, device id for Apple GPUs
    if (strstr(m_Desc.adapterDesc.name, "Apple")) {
        m_Desc.adapterDesc.vendor = nri::Vendor::APPLE;
    }
    
    const uint64_t regID = [m_Device registryID];
    if (regID)
    {
        io_registry_entry_t entry = IOServiceGetMatchingService(MACH_PORT_NULL, IORegistryEntryIDMatching(regID));
        if (entry)
        {
            //         That returned the IOGraphicsAccelerator nub. Its parent, then, is the actual PCI device.
            io_registry_entry_t deviceEntry;
            if (IORegistryEntryGetParentEntry(entry, kIOServicePlane, &deviceEntry) == kIOReturnSuccess)
            {
                m_Desc.adapterDesc.vendor = GetVendorFromID(GetEntryProperty(deviceEntry, CFSTR("vendor-id"))) ;
                m_Desc.adapterDesc.deviceId = GetEntryProperty(deviceEntry, CFSTR("device-id"));
            }
        }
    } else {
        m_Desc.adapterDesc.vendor = nri::Vendor::APPLE;
    }
    

    MTLArgumentBuffersTier argumentBufferTier = MTLArgumentBuffersTier1;
    if ([m_Device respondsToSelector: @selector(argumentBuffersSupport)]) {
        argumentBufferTier = [m_Device argumentBuffersSupport];
    }
    
    for (uint32_t sc = 1; sc <= 64; sc <<= 1) {
        if([m_Device supportsTextureSampleCount: sc]) {
            m_Desc.colorSampleMaxNum |= sc;
            m_Desc.depthSampleMaxNum  |= sc;
            m_Desc.stencilSampleMaxNum  |= sc;
            m_Desc.zeroAttachmentsSampleMaxNum |= sc;
            m_Desc.textureColorSampleMaxNum  |= sc;
            m_Desc.textureIntegerSampleMaxNum |= sc;
            m_Desc.textureDepthSampleMaxNum |= sc;
            m_Desc.textureStencilSampleMaxNum |= sc;
            m_Desc.storageTextureSampleMaxNum |= sc;
        }
    }
    
    
    m_Desc.viewportMaxNum = 16;
    m_Desc.viewportBoundsRange[0] = -32768;
    m_Desc.viewportBoundsRange[1] = 32767;

    m_Desc.attachmentMaxDim = 16384;
    m_Desc.attachmentLayerMaxNum = 2048;
    m_Desc.colorAttachmentMaxNum = 8;

    m_Desc.colorSampleMaxNum = 32;
    m_Desc.depthSampleMaxNum = 32;
    m_Desc.stencilSampleMaxNum = 32;
    m_Desc.zeroAttachmentsSampleMaxNum = 32;
    m_Desc.textureColorSampleMaxNum = 32;
    m_Desc.textureIntegerSampleMaxNum = 32;
    m_Desc.textureDepthSampleMaxNum = 32;
    m_Desc.textureStencilSampleMaxNum = 32;
    m_Desc.storageTextureSampleMaxNum = 32;

    m_Desc.textureArrayLayerMaxNum = 16384;
    m_Desc.typedBufferMaxDim = uint32_t(-1);

    m_Desc.deviceUploadHeapSize = 256 * 1024 * 1024;
    m_Desc.memoryAllocationMaxNum = uint32_t(-1);
    m_Desc.samplerAllocationMaxNum = 4096;
    m_Desc.constantBufferMaxRange = 64 * 1024;
    m_Desc.storageBufferMaxRange = uint32_t(-1);
    m_Desc.bufferTextureGranularity = 1;
    m_Desc.bufferMaxSize = [m_Device maxBufferLength];

    m_Desc.uploadBufferTextureRowAlignment = 1;
    m_Desc.uploadBufferTextureSliceAlignment = 1;
    m_Desc.bufferShaderResourceOffsetAlignment = 1;
    m_Desc.constantBufferOffsetAlignment = 1;
    m_Desc.shaderBindingTableAlignment = 1;
    m_Desc.scratchBufferOffsetAlignment = 1;

    m_Desc.pipelineLayoutDescriptorSetMaxNum = 64;
    m_Desc.pipelineLayoutRootConstantMaxSize = 256;
    m_Desc.pipelineLayoutRootDescriptorMaxNum = 64;

    m_Desc.perStageDescriptorSamplerMaxNum = 1000000;
    m_Desc.perStageDescriptorConstantBufferMaxNum = 1000000;
    m_Desc.perStageDescriptorStorageBufferMaxNum = 1000000;
    m_Desc.perStageDescriptorTextureMaxNum = 1000000;
    m_Desc.perStageDescriptorStorageTextureMaxNum = 1000000;
    m_Desc.perStageResourceMaxNum = 1000000;

    m_Desc.descriptorSetSamplerMaxNum = m_Desc.perStageDescriptorSamplerMaxNum;
    m_Desc.descriptorSetConstantBufferMaxNum = m_Desc.perStageDescriptorConstantBufferMaxNum;
    m_Desc.descriptorSetStorageBufferMaxNum = m_Desc.perStageDescriptorStorageBufferMaxNum;
    m_Desc.descriptorSetTextureMaxNum = m_Desc.perStageDescriptorTextureMaxNum;
    m_Desc.descriptorSetStorageTextureMaxNum = m_Desc.perStageDescriptorStorageTextureMaxNum;

    m_Desc.vertexShaderAttributeMaxNum = 32;
    m_Desc.vertexShaderStreamMaxNum = 32;
    m_Desc.vertexShaderOutputComponentMaxNum = 128;

    m_Desc.tessControlShaderGenerationMaxLevel = 64.0f;
    m_Desc.tessControlShaderPatchPointMaxNum = 32;
    m_Desc.tessControlShaderPerVertexInputComponentMaxNum = 128;
    m_Desc.tessControlShaderPerVertexOutputComponentMaxNum = 128;
    m_Desc.tessControlShaderPerPatchOutputComponentMaxNum = 128;
    m_Desc.tessControlShaderTotalOutputComponentMaxNum = m_Desc.tessControlShaderPatchPointMaxNum * m_Desc.tessControlShaderPerVertexOutputComponentMaxNum + m_Desc.tessControlShaderPerPatchOutputComponentMaxNum;

    m_Desc.tessEvaluationShaderInputComponentMaxNum = 128;
    m_Desc.tessEvaluationShaderOutputComponentMaxNum = 128;

    m_Desc.geometryShaderInvocationMaxNum = 32;
    m_Desc.geometryShaderInputComponentMaxNum = 128;
    m_Desc.geometryShaderOutputComponentMaxNum = 128;
    m_Desc.geometryShaderOutputVertexMaxNum = 1024;
    m_Desc.geometryShaderTotalOutputComponentMaxNum = 1024;

    m_Desc.fragmentShaderInputComponentMaxNum = 128;
    m_Desc.fragmentShaderOutputAttachmentMaxNum = 8;
    m_Desc.fragmentShaderDualSourceAttachmentMaxNum = 1;

    
//    m_Desc.computeShaderSharedMemoryMaxSize = [m_Device maxThreadgroupMemoryLength];
//    const MTLSize threadGroupSize = [m_Device maxThreadsPerThreadgroup];
//    //familiesm_Desc.computeShaderWorkGroupMaxNum = threadGroupSize.width;
//    m_Desc.computeShaderWorkGroupMaxNum[0] = threadGroupSize.width;
//    m_Desc.computeShaderWorkGroupMaxNum[1] = threadGroupSize.height;
//    m_Desc.computeShaderWorkGroupMaxNum[2] = threadGroupSize.depth;
    m_Desc.computeShaderSharedMemoryMaxSize = 64 * 1024;
    m_Desc.computeShaderWorkGroupMaxNum[0] = 64 * 1024;
    m_Desc.computeShaderWorkGroupMaxNum[1] = 64 * 1024;
    m_Desc.computeShaderWorkGroupMaxNum[2] = 64 * 1024;
    m_Desc.computeShaderWorkGroupInvocationMaxNum = 64 * 1024;
    m_Desc.computeShaderWorkGroupMaxDim[0] = 64 * 1024;
    m_Desc.computeShaderWorkGroupMaxDim[1] = 64 * 1024;
    m_Desc.computeShaderWorkGroupMaxDim[2] = 64 * 1024;

    m_Desc.rayTracingShaderGroupIdentifierSize = 32;
    m_Desc.rayTracingShaderTableMaxStride = (uint32_t)(-1);
    m_Desc.rayTracingShaderRecursionMaxDepth = 31;
    m_Desc.rayTracingGeometryObjectMaxNum = (uint32_t)(-1);

    m_Desc.meshControlSharedMemoryMaxSize = 64 * 1024;
    m_Desc.meshControlWorkGroupInvocationMaxNum = 128;
    m_Desc.meshControlPayloadMaxSize = 64 * 1024;
    m_Desc.meshEvaluationOutputVerticesMaxNum = 256;
    m_Desc.meshEvaluationOutputPrimitiveMaxNum = 256;
    m_Desc.meshEvaluationOutputComponentMaxNum = 128;
    m_Desc.meshEvaluationSharedMemoryMaxSize = 64 * 1024;
    m_Desc.meshEvaluationWorkGroupInvocationMaxNum = 128;

    m_Desc.viewportPrecisionBits = 8;
    m_Desc.subPixelPrecisionBits = 8;
    m_Desc.subTexelPrecisionBits = 8;
    m_Desc.mipmapPrecisionBits = 8;

    m_Desc.drawIndirectMaxNum = uint32_t(-1);
    m_Desc.samplerLodBiasMin = -16.0f;
    m_Desc.samplerLodBiasMax = 16.0f;
    m_Desc.samplerAnisotropyMax = 16;
    m_Desc.texelOffsetMin = -8;
    m_Desc.texelOffsetMax = 7;
    m_Desc.texelGatherOffsetMin = -8;
    m_Desc.texelGatherOffsetMax = 7;
    m_Desc.clipDistanceMaxNum = 8;
    m_Desc.cullDistanceMaxNum = 8;
    m_Desc.combinedClipAndCullDistanceMaxNum = 8;
    m_Desc.shadingRateAttachmentTileSize = 16;
    m_Desc.shaderModel = 69;

    m_Desc.conservativeRasterTier = 3;
    m_Desc.sampleLocationsTier = 2;
    m_Desc.shadingRateTier = 2;
    m_Desc.bindlessTier = 2;
    m_Desc.bindlessTier = 2;

    m_Desc.isRayTracingSupported = [m_Device supportsRaytracing];
    
    const MTLGPUFamily appleFamilyGPUs[] = {
        MTLGPUFamilyApple9,
        MTLGPUFamilyApple8,
        MTLGPUFamilyApple7,
        MTLGPUFamilyApple6,
        MTLGPUFamilyApple5,
        MTLGPUFamilyApple4,
        MTLGPUFamilyApple3,
        MTLGPUFamilyApple2,
        MTLGPUFamilyApple1
    };
    
    const MTLGPUFamily macFamilyGPUs[] = {
        MTLGPUFamilyMac2
    };
    
    // https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
    MTLGPUFamily maxSelectedGPU;
    m_Desc.texture3DMaxDim = 2048;
    if(FindMTLGpuFamily(m_Device, appleFamilyGPUs, GetCountOf(appleFamilyGPUs), &maxSelectedGPU)) {
        m_Desc.texture2DMaxDim = (maxSelectedGPU >= MTLGPUFamilyApple3) ? 16384 : 8192;
        m_Desc.texture1DMaxDim = (maxSelectedGPU >= MTLGPUFamilyApple3) ? 16384 : 8192;
        
    } else if(FindMTLGpuFamily(m_Device, macFamilyGPUs, GetCountOf(macFamilyGPUs), &maxSelectedGPU)) {
        m_Desc.texture2DMaxDim = 16384;
        m_Desc.texture1DMaxDim = 16384;
    } else {
        return Result::UNSUPPORTED;
    }
   
    
    return Result::SUCCESS;

}

#include "DeviceMTL.hpp"

