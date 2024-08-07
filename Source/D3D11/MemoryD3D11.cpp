// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"

#include "MemoryD3D11.h"

using namespace nri;

MemoryD3D11::MemoryD3D11(DeviceD3D11& device, const AllocateMemoryDesc& allocateMemoryDesc) : m_Device(device) {
    m_Location = (MemoryLocation)allocateMemoryDesc.type;

    if (allocateMemoryDesc.priority != 0.0f) {
        float p = allocateMemoryDesc.priority * 0.5f + 0.5f;
        float level = 0.0f;

        if (p < 0.2f) {
            m_Priority = DXGI_RESOURCE_PRIORITY_MINIMUM;
            level = 0.0f;
        } else if (p < 0.4f) {
            m_Priority = DXGI_RESOURCE_PRIORITY_LOW;
            level = 0.2f;
        } else if (p < 0.6f) {
            m_Priority = DXGI_RESOURCE_PRIORITY_NORMAL;
            level = 0.4f;
        } else if (p < 0.8f) {
            m_Priority = DXGI_RESOURCE_PRIORITY_HIGH;
            level = 0.6f;
        } else {
            m_Priority = DXGI_RESOURCE_PRIORITY_MAXIMUM;
            level = 0.8f;
        }

        uint32_t bonus = uint32_t(((p - level) / 0.2f) * 65535.0f);
        if (bonus > 0xFFFF)
            bonus = 0xFFFF;

        m_Priority |= bonus;
    }
}
