// © 2021 NVIDIA Corporation

#pragma once

namespace nri {
struct DeviceBase {
    inline DeviceBase(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator)
        : m_CallbackInterface(callbacks)
        , m_StdAllocator(stdAllocator) {
    }

    inline StdAllocator<uint8_t>& GetStdAllocator() {
        return m_StdAllocator;
    }

    const DeviceDesc& GetDesc() const {
        return m_Desc;
    }

    void ReportMessage(Message messageType, const char* file, uint32_t line, const char* format, ...) const;

    virtual ~DeviceBase() {
    }

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
    CallbackInterface m_CallbackInterface = {};
    StdAllocator<uint8_t> m_StdAllocator;
    DeviceDesc m_Desc = {};
};
} // namespace nri
