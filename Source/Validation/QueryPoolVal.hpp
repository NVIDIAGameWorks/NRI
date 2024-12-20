// © 2021 NVIDIA Corporation

QueryPoolVal::QueryPoolVal(DeviceVal& device, QueryPool* queryPool, QueryType queryType, uint32_t queryNum)
    : DeviceObjectVal(device, queryPool)
    , m_QueryType(queryType) {
    m_QueryNum = queryNum;
}

void QueryPoolVal::ResetQueries(uint32_t offset, uint32_t num) {
    GetCoreInterface().ResetQueries(*GetImpl(), offset, num);
}

NRI_INLINE void QueryPoolVal::SetDebugName(const char* name) {
    m_Name = name;
    GetCoreInterface().SetQueryPoolDebugName(*GetImpl(), name);
}

NRI_INLINE uint32_t QueryPoolVal::GetQuerySize() const {
    return GetCoreInterface().GetQuerySize(*GetImpl());
}
