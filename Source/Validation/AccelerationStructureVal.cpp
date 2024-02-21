// © 2021 NVIDIA Corporation

#include "SharedExternal.h"
#include "SharedVal.h"

#include "AccelerationStructureVal.h"
#include "DescriptorVal.h"
#include "DeviceVal.h"
#include "MemoryVal.h"

using namespace nri;

AccelerationStructureVal::~AccelerationStructureVal() {
    if (m_Memory != nullptr)
        m_Memory->UnbindAccelerationStructure(*this);

    m_RayTracingAPI.DestroyAccelerationStructure(*GetImpl());
}

void AccelerationStructureVal::GetMemoryInfo(MemoryDesc& memoryDesc) const {
    m_RayTracingAPI.GetAccelerationStructureMemoryInfo(*GetImpl(), memoryDesc);
    m_Device.RegisterMemoryType(memoryDesc.type, MemoryLocation::DEVICE);
}

uint64_t AccelerationStructureVal::GetUpdateScratchBufferSize() const {
    return m_RayTracingAPI.GetAccelerationStructureUpdateScratchBufferSize(*GetImpl());
}

uint64_t AccelerationStructureVal::GetBuildScratchBufferSize() const {
    return m_RayTracingAPI.GetAccelerationStructureBuildScratchBufferSize(*GetImpl());
}

uint64_t AccelerationStructureVal::GetHandle(uint32_t nodeIndex) const {
    RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), 0, "GetAccelerationStructureHandle: AccelerationStructure is not bound to memory");

    return m_RayTracingAPI.GetAccelerationStructureHandle(*GetImpl(), nodeIndex);
}

uint64_t AccelerationStructureVal::GetNativeObject(uint32_t nodeIndex) const {
    RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), 0, "GetAccelerationStructureNativeObject: AccelerationStructure is not bound to memory");

    return m_RayTracingAPI.GetAccelerationStructureNativeObject(*GetImpl(), nodeIndex);
}

Result AccelerationStructureVal::CreateDescriptor(uint32_t nodeIndex, Descriptor*& descriptor) {
    Descriptor* descriptorImpl = nullptr;
    const Result result = m_RayTracingAPI.CreateAccelerationStructureDescriptor(*GetImpl(), nodeIndex, descriptorImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(&m_Device, descriptorImpl != nullptr, Result::FAILURE, "CreateAccelerationStructureDescriptor: 'impl' is NULL");
        descriptor = (Descriptor*)Allocate<DescriptorVal>(m_Device.GetStdAllocator(), m_Device, descriptorImpl, ResourceType::ACCELERATION_STRUCTURE);
    }

    return result;
}

void AccelerationStructureVal::SetDebugName(const char* name) {
    m_Name = name;
    m_RayTracingAPI.SetAccelerationStructureDebugName(*GetImpl(), name);
}

#include "AccelerationStructureVal.hpp"
