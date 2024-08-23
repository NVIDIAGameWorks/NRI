// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"

#include "BufferD3D11.h"
#include "MemoryD3D11.h"
#include "QueryPoolD3D11.h"
#include "TextureD3D11.h"

using namespace nri;

BufferD3D11::~BufferD3D11() {
    Destroy(m_Device.GetStdAllocator(), m_ReadbackTexture);
}

Result BufferD3D11::Create(MemoryLocation memoryLocation, float priority) {
    // Buffer was already created externally
    if (m_Buffer)
        return Result::SUCCESS;

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (uint32_t)m_Desc.size;
    desc.StructureByteStride = m_Desc.structureStride;

    // D3D11/D3D12 backends do not use FLAG_RAW for descriptors. It seems to be unnecessary because VK doesn't have
    // RAW functionality, but "byte adress buffers" in HLSL do work in VK if the underlying buffer is a structured
    // buffer with stride = 4. Since "ALLOW_RAW_VIEWS" is needed only for D3D11, add it here.
    if (m_Desc.structureStride == 4)
        desc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

    if (m_Desc.structureStride)
        desc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    if (m_Desc.usageMask & BufferUsageBits::ARGUMENT_BUFFER)
        desc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;

    if (memoryLocation == MemoryLocation::HOST_UPLOAD || memoryLocation == MemoryLocation::DEVICE_UPLOAD) {
        if (m_Desc.usageMask == BufferUsageBits::NONE) { // needed for "UploadBufferToTexture"
            desc.Usage = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
        } else {
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        }
    } else if (memoryLocation == MemoryLocation::HOST_READBACK) {
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    } else {
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;
    }

    if (m_Desc.usageMask & BufferUsageBits::VERTEX_BUFFER)
        desc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;

    if (m_Desc.usageMask & BufferUsageBits::INDEX_BUFFER)
        desc.BindFlags |= D3D11_BIND_INDEX_BUFFER;

    if (m_Desc.usageMask & BufferUsageBits::CONSTANT_BUFFER)
        desc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;

    if (m_Desc.usageMask & BufferUsageBits::SHADER_RESOURCE)
        desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

    if (m_Desc.usageMask & BufferUsageBits::SHADER_RESOURCE_STORAGE)
        desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

    HRESULT hr = m_Device->CreateBuffer(&desc, nullptr, &m_Buffer);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateBuffer()");

    // Priority
    uint32_t evictionPriority = ConvertPriority(priority);
    if (evictionPriority != 0)
        m_Buffer->SetEvictionPriority(evictionPriority);

    return Result::SUCCESS;
}

Result BufferD3D11::Create(const BufferDesc& bufferDesc) {
    m_Desc = bufferDesc;

    return Result::SUCCESS;
}

Result BufferD3D11::Create(const BufferD3D11Desc& bufferDesc) {
    if (bufferDesc.desc)
        m_Desc = *bufferDesc.desc;
    else if (!GetBufferDesc(bufferDesc, m_Desc))
        return Result::INVALID_ARGUMENT;

    m_Buffer = (ID3D11Buffer*)bufferDesc.d3d11Resource;

    return Result::SUCCESS;
}

void* BufferD3D11::Map(uint64_t offset) {
    D3D11_MAPPED_SUBRESOURCE mappedData = {};
    m_Device.EnterCriticalSection();
    {
        FinalizeQueries();
        FinalizeReadback();

        D3D11_BUFFER_DESC desc = {};
        m_Buffer->GetDesc(&desc);

        D3D11_MAP map = D3D11_MAP_WRITE;
        if (desc.CPUAccessFlags == D3D11_CPU_ACCESS_WRITE)
            map = desc.Usage == D3D11_USAGE_DYNAMIC ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE;
        else if (desc.CPUAccessFlags == D3D11_CPU_ACCESS_READ)
            map = D3D11_MAP_READ;
        else if (desc.CPUAccessFlags == (D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE))
            map = D3D11_MAP_READ_WRITE; // needed for "UploadBufferToTexture"
        else
            CHECK(false, "Unmappable");

        HRESULT hr = m_Device.GetImmediateContext()->Map(m_Buffer, 0, map, 0, &mappedData);
        if (FAILED(hr)) {
            REPORT_ERROR(&m_Device, "ID3D11DeviceContext::Map() - FAILED!");
            offset = 0;
        }
    }
    m_Device.LeaveCriticalSection();

    uint8_t* ptr = (uint8_t*)mappedData.pData;

    return ptr + offset;
}

void BufferD3D11::FinalizeQueries() {
    if (!m_QueryRange.pool)
        return;

    m_Device.EnterCriticalSection();
    {
        D3D11_MAPPED_SUBRESOURCE mappedData = {};
        HRESULT hr = m_Device.GetImmediateContext()->Map(m_Buffer, 0, D3D11_MAP_WRITE, 0, &mappedData);
        if (SUCCEEDED(hr)) {
            uint8_t* ptr = (uint8_t*)mappedData.pData;
            ptr += m_QueryRange.bufferOffset;

            m_QueryRange.pool->GetData(ptr, m_QueryRange.offset, m_QueryRange.num);

            m_Device.GetImmediateContext()->Unmap(m_Buffer, 0);
        } else
            REPORT_ERROR(&m_Device, "ID3D11DeviceContext::Map() - FAILED!");
    }
    m_Device.LeaveCriticalSection();

    m_QueryRange.pool = nullptr;
}

void BufferD3D11::FinalizeReadback() {
    if (!m_IsReadbackDataChanged)
        return;

    m_IsReadbackDataChanged = false;

    m_Device.EnterCriticalSection();
    {
        D3D11_MAPPED_SUBRESOURCE srcData = {};
        HRESULT hr = m_Device.GetImmediateContext()->Map(*m_ReadbackTexture, 0, D3D11_MAP_READ, 0, &srcData);
        if (FAILED(hr)) {
            REPORT_ERROR(&m_Device, "ID3D11DeviceContext::Map() - FAILED!");
            return;
        }

        D3D11_MAPPED_SUBRESOURCE dstData = {};
        hr = m_Device.GetImmediateContext()->Map(m_Buffer, 0, D3D11_MAP_WRITE, 0, &dstData);
        if (FAILED(hr)) {
            m_Device.GetImmediateContext()->Unmap(*m_ReadbackTexture, 0);
            REPORT_ERROR(&m_Device, "ID3D11DeviceContext::Map() - FAILED!");
            return;
        }

        const uint32_t d = m_ReadbackTexture->GetDesc().depth;
        const uint32_t h = m_ReadbackTexture->GetDesc().height;
        const uint8_t* src = (uint8_t*)srcData.pData;
        uint8_t* dst = (uint8_t*)dstData.pData;
        for (uint32_t i = 0; i < d; i++) {
            for (uint32_t j = 0; j < h; j++) {
                const uint8_t* s = src + j * srcData.RowPitch;
                uint8_t* dstLocal = dst + j * m_ReadbackDataLayoutDesc.rowPitch;
                memcpy(dstLocal, s, srcData.RowPitch);
            }
            src += srcData.DepthPitch;
            dst += m_ReadbackDataLayoutDesc.slicePitch;
        }

        m_Device.GetImmediateContext()->Unmap(m_Buffer, 0);
        m_Device.GetImmediateContext()->Unmap(*m_ReadbackTexture, 0);
    }
    m_Device.LeaveCriticalSection();
}

TextureD3D11& BufferD3D11::RecreateReadbackTexture(const TextureD3D11& srcTexture, const TextureRegionDesc& srcRegionDesc, const TextureDataLayoutDesc& readbackDataLayoutDesc) {
    bool isChanged = true;
    if (m_ReadbackTexture) {
        const TextureDesc& curr = m_ReadbackTexture->GetDesc();
        isChanged = curr.format != srcTexture.GetDesc().format || curr.width != srcRegionDesc.width || curr.height != srcRegionDesc.height || curr.depth != srcRegionDesc.depth;
    }

    if (isChanged) {
        TextureDesc textureDesc = {};
        textureDesc.mipNum = 1;
        textureDesc.sampleNum = 1;
        textureDesc.arraySize = 1;
        textureDesc.format = srcTexture.GetDesc().format;
        textureDesc.width = srcRegionDesc.width;
        textureDesc.height = srcRegionDesc.height;
        textureDesc.depth = srcRegionDesc.depth;

        textureDesc.type = TextureType::TEXTURE_2D;
        if (srcRegionDesc.depth > 1)
            textureDesc.type = TextureType::TEXTURE_3D;
        else if (srcRegionDesc.height == 1)
            textureDesc.type = TextureType::TEXTURE_1D;

        Destroy(m_Device.GetStdAllocator(), m_ReadbackTexture);

        m_Device.CreateImplementation<TextureD3D11>(m_ReadbackTexture, textureDesc);
        if (m_ReadbackTexture)
            m_ReadbackTexture->Create(MemoryLocation::HOST_READBACK, 0.0f);
    }

    m_IsReadbackDataChanged = true;
    m_ReadbackDataLayoutDesc = readbackDataLayoutDesc;

    return *m_ReadbackTexture;
}

//================================================================================================================
// NRI
//================================================================================================================

void BufferD3D11::Unmap() {
    m_Device.EnterCriticalSection();
    m_Device.GetImmediateContext()->Unmap(m_Buffer, 0);
    m_Device.LeaveCriticalSection();
}

#include "BufferD3D11.hpp"
