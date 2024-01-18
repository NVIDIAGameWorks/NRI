#pragma once

constexpr size_t LOCK_CACHELINE_SIZE = 64;

// Very lightweight exclusive lock
struct alignas(LOCK_CACHELINE_SIZE) Lock
{
    inline Lock()
    { m_Atomic.store(0, std::memory_order_relaxed); }

    inline void Acquire()
    {
        while (m_Atomic.exchange(1, std::memory_order_acquire))
            _mm_pause();
    }

    inline void Release()
    { m_Atomic.store(0, std::memory_order_release); }

private:
    std::atomic_uint32_t m_Atomic;
};

struct ExclusiveScope
{
    inline ExclusiveScope(Lock& lock) :
        m_Lock(lock)
    { m_Lock.Acquire(); }

    inline ~ExclusiveScope()
    { m_Lock.Release(); }

private:
    Lock& m_Lock;
};