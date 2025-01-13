#pragma once

struct BufferUpdateRequest {
    nri::BufferUpdateRequestDesc desc;
    uint64_t offset;
};

struct TextureUpdateRequest {
    nri::TextureUpdateRequestDesc desc;
    uint64_t offset;
};

struct GarbageInFlight {
    nri::Buffer* buffer;
    nri::Memory* memory;
    uint32_t frameNum;
};

struct StreamerImpl : public nri::DebugNameBase {
    inline StreamerImpl(nri::Device& device, const nri::CoreInterface& NRI)
        : m_Device(device)
        , m_NRI(NRI)
        , m_BufferRequests(((nri::DeviceBase&)device).GetStdAllocator())
        , m_BufferRequestsWithDst(((nri::DeviceBase&)device).GetStdAllocator())
        , m_TextureRequests(((nri::DeviceBase&)device).GetStdAllocator())
        , m_TextureRequestsWithDst(((nri::DeviceBase&)device).GetStdAllocator())
        , m_GarbageInFlight(((nri::DeviceBase&)device).GetStdAllocator()) {
    }

    inline nri::Buffer* GetDynamicBuffer() {
        return m_DynamicBuffer;
    }

    inline nri::Buffer* GetConstantBuffer() {
        return m_ConstantBuffer;
    }

    inline nri::Device& GetDevice() {
        return m_Device;
    }

    ~StreamerImpl();

    nri::Result Create(const nri::StreamerDesc& desc);
    uint32_t UpdateStreamerConstantBuffer(const void* data, uint32_t dataSize);
    uint64_t AddStreamerBufferUpdateRequest(const nri::BufferUpdateRequestDesc& bufferUpdateRequestDesc);
    uint64_t AddStreamerTextureUpdateRequest(const nri::TextureUpdateRequestDesc& textureUpdateRequestDesc);
    nri::Result CopyStreamerUpdateRequests();
    void CmdUploadStreamerUpdateRequests(nri::CommandBuffer& commandBuffer);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) override {
        m_NRI.SetDebugName(m_ConstantBuffer, name);
        m_NRI.SetDebugName(m_ConstantBufferMemory, name);
        m_NRI.SetDebugName(m_DynamicBuffer, name);
        m_NRI.SetDebugName(m_DynamicBufferMemory, name);
    }

private:
    nri::Device& m_Device;
    const nri::CoreInterface& m_NRI;
    nri::StreamerDesc m_Desc = {};
    Vector<BufferUpdateRequest> m_BufferRequests;
    Vector<BufferUpdateRequest> m_BufferRequestsWithDst;
    Vector<TextureUpdateRequest> m_TextureRequests;
    Vector<TextureUpdateRequest> m_TextureRequestsWithDst;
    Vector<GarbageInFlight> m_GarbageInFlight;
    nri::Buffer* m_ConstantBuffer = nullptr;
    nri::Memory* m_ConstantBufferMemory = nullptr;
    nri::Buffer* m_DynamicBuffer = nullptr;
    nri::Memory* m_DynamicBufferMemory = nullptr;
    uint32_t m_ConstantDataOffset = 0;
    uint64_t m_DynamicDataOffset = 0;
    uint64_t m_DynamicDataOffsetBase = 0;
    uint64_t m_DynamicBufferSize = 0;
    uint32_t m_FrameIndex = 0;
};