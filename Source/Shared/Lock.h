// © 2021 NVIDIA Corporation

#pragma once

constexpr size_t CACHELINE_SIZE = 64;

// Very lightweight read/write lock
struct alignas(CACHELINE_SIZE) Lock
{
    Lock();

    void ExclusiveLock();
    void ExclusiveUnlock();
    void SharedLock();
    void SharedUnlock();

private:
    std::atomic_int64_t m_Atomic;
};

inline Lock::Lock()
{
    m_Atomic.store(0, std::memory_order_relaxed);
}

inline void Lock::ExclusiveLock()
{
    int64_t expected = 0;
    while (!m_Atomic.compare_exchange_weak(expected, -1, std::memory_order_acquire))
        expected = 0;
}

inline void Lock::ExclusiveUnlock()
{
    m_Atomic.store(0, std::memory_order_release);
}

inline void Lock::SharedLock()
{
    int64_t expected = 0;
    while (!m_Atomic.compare_exchange_weak(expected, expected + 1, std::memory_order_acquire))
        expected = std::max<int64_t>(expected, 0);
}

inline void Lock::SharedUnlock()
{
    m_Atomic.fetch_sub(1, std::memory_order_release);
}

struct ExclusiveScope
{
    ExclusiveScope(Lock& lock);
    ~ExclusiveScope();

private:
    Lock& m_Lock;
};

inline ExclusiveScope::ExclusiveScope(Lock& lock) : m_Lock(lock)
{
    m_Lock.ExclusiveLock();
}

inline ExclusiveScope::~ExclusiveScope()
{
    m_Lock.ExclusiveUnlock();
}

struct SharedScope
{
    SharedScope(Lock& lock);
    ~SharedScope();

private:
    Lock& m_Lock;
};

inline SharedScope::SharedScope(Lock& lock) : m_Lock(lock)
{
    m_Lock.SharedLock();
}

inline SharedScope::~SharedScope()
{
    m_Lock.SharedUnlock();
}
