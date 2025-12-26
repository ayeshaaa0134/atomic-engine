#include "primitives.h"
#include <immintrin.h>
#include <atomic>

#ifdef _WIN32
#include <intrin.h>
#endif

namespace atomic_tree {

void pmem_flush(void* addr, std::size_t len) {
    char* ptr = static_cast<char*>(addr);
    const size_t cache_line_size = 64;
    uintptr_t start = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t end = start + len;
    start = start & ~(cache_line_size - 1);
    
    for (uintptr_t p = start; p < end; p += cache_line_size) {
        _mm_clflush(reinterpret_cast<void*>(p));
    }
}

void pmem_fence() {
    _mm_sfence();
}

void persist(void* addr, std::size_t len) {
    pmem_flush(addr, len);
    pmem_fence();
}

void atomic_pointer_swap(std::uint64_t* addr, std::uint64_t new_value, 
                         std::uint64_t* out_old_value) {
#ifdef _WIN32
    *out_old_value = static_cast<std::uint64_t>(
        _InterlockedExchange64(reinterpret_cast<volatile __int64*>(addr), 
                               static_cast<__int64>(new_value))
    );
#else
    *out_old_value = __atomic_exchange_n(addr, new_value, __ATOMIC_SEQ_CST);
#endif
    // Persist the pointer update itself?
    // The paper says: Write Data -> Flush -> Fence -> Update Pointer.
    // The pointer update is an 8-byte atomic store.
    // It is visible to other cores immediately.
    // To survive power loss, it must reach NVM.
    // So we flush the pointer address too.
    persist(addr, sizeof(std::uint64_t));
}

} // namespace atomic_tree
