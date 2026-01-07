#include "primitives.h"

#include <cstdint>
#include <cstddef>

#ifdef _WIN32
#    include <windows.h>
#endif

namespace atomic_tree {

std::uint64_t total_persisted_bytes = 0;

void pmem_flush(void *addr, std::size_t len) {
    total_persisted_bytes += len;

    // Align down to 64-byte cache line.
    char *ptr = reinterpret_cast<char *>(
        reinterpret_cast<std::uintptr_t>(addr) & ~std::uintptr_t{63}
    );

    char *end = static_cast<char *>(addr) + len;

    for (; ptr < end; ptr += 64) {
        _mm_clflush(ptr);
    }
}

void pmem_fence() noexcept {
    _mm_sfence();
}

void persist(void *addr, std::size_t len) {
    pmem_flush(addr, len);
    pmem_fence();
}

void atomic_pointer_swap(std::uint64_t *addr,
                         std::uint64_t new_value,
                         std::uint64_t *out_old_value) noexcept {
#ifdef _WIN32
    // InterlockedExchange64 expects a volatile LONG64* on Windows.
    std::uint64_t old =
        static_cast<std::uint64_t>(
            InterlockedExchange64(
                reinterpret_cast<volatile LONG64 *>(addr),
                static_cast<LONG64>(new_value)
            )
        );
    if (out_old_value) [[unlikely]] {
        *out_old_value = old;
    }
#else
    std::uint64_t old = __sync_lock_test_and_set(addr, new_value);
    if (out_old_value) [[unlikely]] {
        *out_old_value = old;
    }
#endif
}

} // namespace atomic_tree
