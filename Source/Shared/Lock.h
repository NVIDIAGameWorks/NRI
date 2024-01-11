// © 2021 NVIDIA Corporation

#pragma once

// Very lightweight read/write lock
constexpr size_t LOCK_CACHELINE_SIZE = 64;
typedef int64_t LOCK_DATA_TYPE;

// TODO: add _mm_pause? yeild?
// see https://timur.audio/using-locks-in-real-time-audio-processing-safely
// and https://github.com/microsoft/STL/issues/680
struct alignas(LOCK_CACHELINE_SIZE) Lock
{
    inline Lock()
    { m_Atomic.store(0, std::memory_order_relaxed); }

    inline void ExclusiveLock()
    {
        LOCK_DATA_TYPE expected = 0;
        while (!m_Atomic.compare_exchange_weak(expected, -1, std::memory_order_acquire))
            expected = 0;
    }

    inline void ExclusiveUnlock()
    { m_Atomic.store(0, std::memory_order_release); }

    inline void SharedLock()
    {
        LOCK_DATA_TYPE expected = 0;
        while (!m_Atomic.compare_exchange_weak(expected, expected + 1, std::memory_order_acquire))
            expected = std::max<LOCK_DATA_TYPE>(expected, 0);
    }

    inline void SharedUnlock()
    { m_Atomic.fetch_sub(1, std::memory_order_release); }

private:
    std::atomic<LOCK_DATA_TYPE> m_Atomic;
};

struct ExclusiveScope
{
    inline ExclusiveScope(Lock& lock) :
        m_Lock(lock)
    { m_Lock.ExclusiveLock(); }

    inline ~ExclusiveScope()
    { m_Lock.ExclusiveUnlock(); }

private:
    Lock& m_Lock;
};

struct SharedScope
{
    inline SharedScope(Lock& lock) :
        m_Lock(lock)
    { m_Lock.SharedLock(); }

    inline ~SharedScope()
    { m_Lock.SharedUnlock(); }

private:
    Lock& m_Lock;
};
