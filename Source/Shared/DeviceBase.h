// © 2021 NVIDIA Corporation

#pragma once

#define NRI_OBJECT_SIGNATURE 0x1234567887654321ull // TODO: 32-bit platform support? not needed, I believe

namespace nri {

struct DebugNameBase {
    virtual void SetDebugName(const char*) {
    }
};

struct DeviceBase : public DebugNameBase {
    inline DeviceBase(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks)
        : m_CallbackInterface(callbacks)
        , m_AllocationCallbacks(allocationCallbacks)
        , m_StdAllocator(m_AllocationCallbacks) {
    }

    inline StdAllocator<uint8_t>& GetStdAllocator() {
        return m_StdAllocator;
    }

    inline const AllocationCallbacks& GetAllocationCallbacks() const {
        return m_AllocationCallbacks;
    }

    void ReportMessage(Message messageType, const char* file, uint32_t line, const char* format, ...) const;

    virtual ~DeviceBase() {
    }

    virtual const DeviceDesc& GetDesc() const = 0;
    virtual void Destruct() = 0;

    virtual Result FillFunctionTable(CoreInterface&) const {
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(HelperInterface&) const {
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(LowLatencyInterface&) const {
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(MeshShaderInterface&) const {
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(RayTracingInterface&) const {
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(StreamerInterface&) const {
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(SwapChainInterface&) const {
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(ResourceAllocatorInterface&) const {
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(WrapperD3D11Interface&) const {
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(WrapperD3D12Interface&) const {
        return Result::UNSUPPORTED;
    }

    virtual Result FillFunctionTable(WrapperVKInterface&) const {
        return Result::UNSUPPORTED;
    }

protected:
#ifndef NDEBUG
    uint64_t m_Signature = NRI_OBJECT_SIGNATURE; // .natvis
#endif
    CallbackInterface m_CallbackInterface = {};
    AllocationCallbacks m_AllocationCallbacks = {};
    StdAllocator<uint8_t> m_StdAllocator;
};

} // namespace nri
