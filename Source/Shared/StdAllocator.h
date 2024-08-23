// © 2021 NVIDIA Corporation

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

template <typename T>
void StdAllocator_MaybeUnused([[maybe_unused]] const T& arg) {
}

#if defined(_WIN32)
#    include <malloc.h>

inline void* AlignedMalloc(void* userArg, size_t size, size_t alignment) {
    StdAllocator_MaybeUnused(userArg);

    return _aligned_malloc(size, alignment);
}

inline void* AlignedRealloc(void* userArg, void* memory, size_t size, size_t alignment) {
    StdAllocator_MaybeUnused(userArg);

    return _aligned_realloc(memory, size, alignment);
}

inline void AlignedFree(void* userArg, void* memory) {
    StdAllocator_MaybeUnused(userArg);

    _aligned_free(memory);
}

#elif defined(__linux__) || defined(__APPLE__)
#    include <alloca.h>
#    include <cstdlib>
#    define _alloca alloca

template <typename T>
inline T Align(T x, size_t alignment);

inline void* AlignedMalloc(void* userArg, size_t size, size_t alignment) {
    StdAllocator_MaybeUnused(userArg);

    uint8_t* memory = (uint8_t*)malloc(size + sizeof(uint8_t*) + alignment - 1);
    if (!memory)
        return nullptr;

    uint8_t* alignedMemory = Align(memory + sizeof(uint8_t*), alignment);
    uint8_t** memoryHeader = (uint8_t**)alignedMemory - 1;
    *memoryHeader = memory;

    return alignedMemory;
}

inline void* AlignedRealloc(void* userArg, void* memory, size_t size, size_t alignment) {
    if (!memory)
        return AlignedMalloc(userArg, size, alignment);

    uint8_t** memoryHeader = (uint8_t**)memory - 1;
    uint8_t* oldMemory = *memoryHeader;

    uint8_t* newMemory = (uint8_t*)realloc(oldMemory, size + sizeof(uint8_t*) + alignment - 1);
    if (!newMemory)
        return nullptr;

    if (newMemory == oldMemory)
        return memory;

    uint8_t* alignedMemory = Align(newMemory + sizeof(uint8_t*), alignment);
    memoryHeader = (uint8_t**)alignedMemory - 1;
    *memoryHeader = newMemory;

    return alignedMemory;
}

inline void AlignedFree(void* userArg, void* memory) {
    StdAllocator_MaybeUnused(userArg);

    if (!memory)
        return;

    uint8_t** memoryHeader = (uint8_t**)memory - 1;
    uint8_t* oldMemory = *memoryHeader;
    free(oldMemory);
}

#endif

inline void CheckAndSetDefaultAllocator(AllocationCallbacks& allocationCallbacks) {
    if (allocationCallbacks.Allocate)
        return;

    allocationCallbacks.Allocate = AlignedMalloc;
    allocationCallbacks.Reallocate = AlignedRealloc;
    allocationCallbacks.Free = AlignedFree;
}

template <typename T>
struct StdAllocator {
    typedef T value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef std::true_type propagate_on_container_move_assignment;
    typedef std::false_type is_always_equal;

    StdAllocator(const AllocationCallbacks& allocationCallbacks) : m_Interface(allocationCallbacks) {
    }

    StdAllocator(const StdAllocator<T>& allocator) : m_Interface(allocator.GetInterface()) {
    }

    template <class U>
    StdAllocator(const StdAllocator<U>& allocator) : m_Interface(allocator.GetInterface()) {
    }

    StdAllocator<T>& operator=(const StdAllocator<T>& allocator) {
        m_Interface = allocator.GetInterface();
        return *this;
    }

    T* allocate(size_t n) noexcept {
        return (T*)m_Interface.Allocate(m_Interface.userArg, n * sizeof(T), alignof(T));
    }

    void deallocate(T* memory, size_t) noexcept {
        m_Interface.Free(m_Interface.userArg, memory);
    }

    const AllocationCallbacks& GetInterface() const {
        return m_Interface;
    }

    template <typename U>
    using other = StdAllocator<U>;

private:
    AllocationCallbacks m_Interface = {};
};

template <typename T>
bool operator==(const StdAllocator<T>& left, const StdAllocator<T>& right) {
    return left.GetInterface() == right.GetInterface();
}

template <typename T>
bool operator!=(const StdAllocator<T>& left, const StdAllocator<T>& right) {
    return !operator==(left, right);
}

//================================================================================================================

template <typename T>
using Vector = std::vector<T, StdAllocator<T>>;

template <typename U, typename T>
using UnorderedMap = std::unordered_map<U, T, std::hash<U>, std::equal_to<U>, StdAllocator<std::pair<const U, T>>>;

using String = std::basic_string<char, std::char_traits<char>, StdAllocator<char>>;

//================================================================================================================

template <typename T>
struct Scratch {
    T* mem;
    const AllocationCallbacks& allocator;
    bool isHeap;

    ~Scratch() {
        if (isHeap)
            allocator.Free(allocator.userArg, mem);
    }

    inline operator T*() const {
        return mem;
    }
};

constexpr size_t MAX_STACK_ALLOC_SIZE = 128 * 1024;

#define AllocateScratch(device, T, elementNum) \
    {((elementNum) * sizeof(T) + alignof(T)) > MAX_STACK_ALLOC_SIZE \
            ? (T*)(device).GetStdAllocator().GetInterface().Allocate((device).GetStdAllocator().GetInterface().userArg, (elementNum) * sizeof(T), alignof(T)) \
            : (T*)Align((elementNum) ? (T*)_alloca(((elementNum) * sizeof(T) + alignof(T))) : nullptr, alignof(T)), \
        (device).GetStdAllocator().GetInterface(), ((elementNum) * sizeof(T) + alignof(T)) > MAX_STACK_ALLOC_SIZE}

#define StackAlloc(T, elementNum) Align(((elementNum) ? (T*)_alloca((elementNum) * sizeof(T) + alignof(T)) : nullptr), alignof(T))
