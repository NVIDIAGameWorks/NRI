// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceMTL;

struct TextureMTL {

    inline TextureMTL(DeviceMTL& device)
        : m_Device(device) {
    }
private:
    DeviceMTL& m_Device;

};

}

