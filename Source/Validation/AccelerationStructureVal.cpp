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

    GetRayTracingInterface().DestroyAccelerationStructure(*GetImpl());
}

void AccelerationStructureVal::GetMemoryInfo(MemoryDesc& memoryDesc) const {
    GetRayTracingInterface().GetAccelerationStructureMemoryInfo(*GetImpl(), memoryDesc);
    m_Device.RegisterMemoryType(memoryDesc.type, MemoryLocation::DEVICE);
}

uint64_t AccelerationStructureVal::GetUpdateScratchBufferSize() const {
    return GetRayTracingInterface().GetAccelerationStructureUpdateScratchBufferSize(*GetImpl());
}

uint64_t AccelerationStructureVal::GetBuildScratchBufferSize() const {
    return GetRayTracingInterface().GetAccelerationStructureBuildScratchBufferSize(*GetImpl());
}

uint64_t AccelerationStructureVal::GetHandle() const {
    RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), 0, "GetAccelerationStructureHandle: AccelerationStructure is not bound to memory");

    return GetRayTracingInterface().GetAccelerationStructureHandle(*GetImpl());
}

uint64_t AccelerationStructureVal::GetNativeObject() const {
    RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), 0, "GetAccelerationStructureNativeObject: AccelerationStructure is not bound to memory");

    return GetRayTracingInterface().GetAccelerationStructureNativeObject(*GetImpl());
}

Result AccelerationStructureVal::CreateDescriptor(Descriptor*& descriptor) {
    Descriptor* descriptorImpl = nullptr;
    const Result result = GetRayTracingInterface().CreateAccelerationStructureDescriptor(*GetImpl(), descriptorImpl);

    if (result == Result::SUCCESS) {
        RETURN_ON_FAILURE(&m_Device, descriptorImpl != nullptr, Result::FAILURE, "CreateAccelerationStructureDescriptor: 'impl' is NULL");
        descriptor = (Descriptor*)Allocate<DescriptorVal>(m_Device.GetStdAllocator(), m_Device, descriptorImpl, ResourceType::ACCELERATION_STRUCTURE);
    }

    return result;
}

void AccelerationStructureVal::SetDebugName(const char* name) {
    m_Name = name;
    GetRayTracingInterface().SetAccelerationStructureDebugName(*GetImpl(), name);
}

#include "AccelerationStructureVal.hpp"
