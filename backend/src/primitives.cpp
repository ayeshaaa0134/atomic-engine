#include "primitives.h"
#include <immintrin.h>
#ifdef _WIN32
#include <windows.h>
#endif

namespace atomic_tree {

std::uint64_t total_persisted_bytes = 0;

void pmem_flush(void *addr, std::size_t len) {
  total_persisted_bytes += len;
  char *ptr = (char *)((uintptr_t)addr & ~(uintptr_t)63);
  for (; ptr < (char *)addr + len; ptr += 64) {
    _mm_clflush(ptr);
  }
}

void pmem_fence() { _mm_sfence(); }

void persist(void *addr, std::size_t len) {
  pmem_flush(addr, len);
  pmem_fence();
}

void atomic_pointer_swap(std::uint64_t *addr, std::uint64_t new_value,
                         std::uint64_t *out_old_value) {
#ifdef _WIN32
  std::uint64_t old =
      InterlockedExchange64((LONG64 volatile *)addr, (LONG64)new_value);
  if (out_old_value)
    *out_old_value = old;
#else
  // Fallback for non-windows if needed, but project is Windows-focused
  std::uint64_t old = __sync_lock_test_and_set(addr, new_value);
  if (out_old_value)
    *out_old_value = old;
#endif
}

} // namespace atomic_tree
