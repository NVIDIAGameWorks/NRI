// © 2021 NVIDIA Corporation

#pragma region [  Core  ]

static void NRI_CALL SetQueryPoolDebugName(QueryPool& queryPool, const char* name)
{
    ((QueryPoolVK&)queryPool).SetDebugName(name);
}

static uint32_t NRI_CALL GetQuerySize(const QueryPool& queryPool)
{
    return ((QueryPoolVK&)queryPool).GetQuerySize();
}

#pragma endregion

Define_Core_QueryPool_PartiallyFillFunctionTable(VK)
