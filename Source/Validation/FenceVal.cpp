// © 2021 NVIDIA Corporation

#include "SharedExternal.h"
#include "SharedVal.h"
#include "FenceVal.h"
#include "CommandQueueVal.h"

using namespace nri;

//================================================================================================================
// NRI
//================================================================================================================

inline uint64_t FenceVal::GetFenceValue() const {
    return GetCoreInterface().GetFenceValue(*GetImpl());
}

inline void FenceVal::QueueSignal(CommandQueueVal& commandQueue, uint64_t value) {
    GetCoreInterface().QueueSignal(*commandQueue.GetImpl(), *GetImpl(), value);
}

inline void FenceVal::QueueWait(CommandQueueVal& commandQueue, uint64_t value) {
    GetCoreInterface().QueueWait(*commandQueue.GetImpl(), *GetImpl(), value);
}

inline void FenceVal::Wait(uint64_t value) {
    GetCoreInterface().Wait(*GetImpl(), value);
}

inline void FenceVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetFenceDebugName(*GetImpl(), name);
}

#include "FenceVal.hpp"
