// © 2021 NVIDIA Corporation

QueryPoolVal::QueryPoolVal(DeviceVal& device, QueryPool* queryPool, QueryType queryType, uint32_t queryNum)
    : DeviceObjectVal(device, queryPool)
    , m_DeviceState(device.GetStdAllocator())
    , m_QueryType(queryType) {
    m_QueryNum = queryNum;

    if (queryNum != 0) {
        const size_t batchNum = std::max(queryNum >> 6, 1u);
        m_DeviceState.resize(batchNum, 0);
    }
}

bool QueryPoolVal::SetQueryState(uint32_t offset, bool state) {
    const size_t batchIndex = offset >> 6;
    const uint64_t batchValue = m_DeviceState[batchIndex];
    const size_t bitIndex = 1ull << (offset & 63);
    const uint64_t maskBitValue = ~bitIndex;
    const uint64_t bitValue = state ? bitIndex : 0;

    m_DeviceState[batchIndex] = (batchValue & maskBitValue) | bitValue;

    return batchValue & bitIndex;
}

void QueryPoolVal::ResetQueries(uint32_t offset, uint32_t number) {
    for (uint32_t i = 0; i < number; i++)
        SetQueryState(offset + i, false);
}

NRI_INLINE void QueryPoolVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetQueryPoolDebugName(*GetImpl(), name);
}

NRI_INLINE uint32_t QueryPoolVal::GetQuerySize() const {
    return GetCoreInterface().GetQuerySize(*GetImpl());
}
