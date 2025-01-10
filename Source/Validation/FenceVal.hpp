// © 2021 NVIDIA Corporation

NRI_INLINE uint64_t FenceVal::GetFenceValue() const {
    return GetCoreInterface().GetFenceValue(*GetImpl());
}

NRI_INLINE void FenceVal::Wait(uint64_t value) {
    GetCoreInterface().Wait(*GetImpl(), value);
}
