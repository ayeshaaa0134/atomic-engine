#ifndef ATOMIC_TREE_PRIMITIVES_H
#define ATOMIC_TREE_PRIMITIVES_H

#include <cstddef>
#include <cstdint>
#include <immintrin.h> // for _mm_clflush, _mm_sfence

namespace atomic_tree {

extern std::uint64_t total_persisted_bytes;

void pmem_flush(void *addr, std::size_t len);

void pmem_fence() noexcept;

void persist(void *addr, std::size_t len);

void atomic_pointer_swap(std::uint64_t *addr,
                         std::uint64_t new_value,
                         std::uint64_t *out_old_value) noexcept;

} // namespace atomic_tree

#endif // ATOMIC_TREE_PRIMITIVES_H
