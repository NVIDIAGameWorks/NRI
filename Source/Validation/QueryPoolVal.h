// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct QueryPoolVal final : public DeviceObjectVal<QueryPool> {
    QueryPoolVal(DeviceVal& device, QueryPool* queryPool, QueryType queryType, uint32_t queryNum);

    void ResetQueries(uint32_t offset, uint32_t num);

    inline uint32_t GetQueryNum() const {
        return m_QueryNum;
    }

    inline QueryType GetQueryType() const {
        return m_QueryType;
    }

    inline bool IsImported() const {
        return m_QueryNum == 0;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    uint32_t GetQuerySize() const;

private:
    uint32_t m_QueryNum;
    QueryType m_QueryType;
};

} // namespace nri
