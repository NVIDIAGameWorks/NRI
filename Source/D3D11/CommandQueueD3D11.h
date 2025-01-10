// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceD3D11;

struct CommandQueueD3D11 final : public DebugNameBase {
    inline CommandQueueD3D11(DeviceD3D11& device)
        : m_Device(device) {
    }

    inline ~CommandQueueD3D11() {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    void Submit(const QueueSubmitDesc& queueSubmitDesc);
    Result UploadData(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum);
    Result WaitForIdle();

private:
    DeviceD3D11& m_Device;
};

} // namespace nri
