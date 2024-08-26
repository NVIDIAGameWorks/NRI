// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceVK;

struct QueryPoolVK {
    inline QueryPoolVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline VkQueryPool GetHandle() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline VkQueryType GetType() const {
        return m_Type;
    }

    ~QueryPoolVK();

    Result Create(const QueryPoolDesc& queryPoolDesc);
    Result Create(const QueryPoolVKDesc& queryPoolDesc);

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);

    inline uint32_t GetQuerySize() const {
        return m_QuerySize;
    }

private:
    DeviceVK& m_Device;
    VkQueryPool m_Handle = VK_NULL_HANDLE;
    VkQueryType m_Type = (VkQueryType)0;
    uint32_t m_QuerySize = 0;
    bool m_OwnsNativeObjects = true;
};

} // namespace nri