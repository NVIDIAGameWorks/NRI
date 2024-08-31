#pragma once

namespace nri {

struct DeviceMTl final : public DeviceBase {
    DeviceMTl(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator);
    ~DeviceMTl();


    Result GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue);

    inline operator id<MTLDevice>&() const {
        return m_Device;
    }

    inline const DeviceDesc& GetDesc() const {
        return m_Desc;
    }
    
    FormatSupportBits GetFormatSupport(const Device& device, Format format);

    void FillCreateInfo(const TextureDesc& bufferDesc, MTLTextureDescriptor* info) const;

    Result FillFunctionTable(CoreInterface& table) const;
    Result FillFunctionTable(HelperInterface& table) const;
    Result FillFunctionTable(LowLatencyInterface& table) const;
    Result FillFunctionTable(MeshShaderInterface& table) const;
    Result FillFunctionTable(RayTracingInterface& table) const;
    Result FillFunctionTable(StreamerInterface& table) const;
    Result FillFunctionTable(SwapChainInterface& table) const;
    Result FillFunctionTable(ResourceAllocatorInterface& table) const;

    Result Create(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationMTLDesc& deviceCreationVKDesc, bool isWrapper);
private:
    DeviceDesc m_Desc = {};
    id<MTLDevice>     m_Device;
    std::array<CommandQueueMTL*, (uint32_t)CommandQueueType::MAX_NUM> m_CommandQueues = {};
    DeviceDesc m_Desc = {};
    MTLGPUFamily m_Family;
    bool m_OwnsNativeObjects = true;
};
}; // namespace nri
