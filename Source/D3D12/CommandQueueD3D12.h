// © 2021 NVIDIA Corporation

#pragma once

struct ID3D12Device;
struct ID3D12CommandQueue;
enum D3D12_COMMAND_LIST_TYPE;

namespace nri {

struct DeviceD3D12;

struct CommandQueueD3D12 {
    inline CommandQueueD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline ~CommandQueueD3D12() {
    }

    inline operator ID3D12CommandQueue*() const {
        return m_CommandQueue.GetInterface();
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline D3D12_COMMAND_LIST_TYPE GetType() const {
        return m_CommandListType;
    }

    Result Create(CommandQueueType commandQueueType);
    Result Create(ID3D12CommandQueue* commandQueue);

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name) {
        SET_D3D_DEBUG_OBJECT_NAME(m_CommandQueue, name);
    }

    void BeginAnnotation(const char* name, uint32_t bgra);
    void EndAnnotation();
    void Annotation(const char* name, uint32_t bgra);
    void Submit(const QueueSubmitDesc& queueSubmitDesc);
    Result UploadData(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum);
    Result WaitForIdle();

private:
    DeviceD3D12& m_Device;
    ComPtr<ID3D12CommandQueue> m_CommandQueue;
    D3D12_COMMAND_LIST_TYPE m_CommandListType = D3D12_COMMAND_LIST_TYPE(-1);
};

} // namespace nri
