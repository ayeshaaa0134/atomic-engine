#ifndef ATOMICTREE_PRIMITIVES_H
#define ATOMICTREE_PRIMITIVES_H

#include <cstddef>
#include <cstdint>

namespace atomic_tree {

// Single cache line flush
void pmem_flush(void* addr, std::size_t len);

// Store fence
void pmem_fence();

// Full persist: Flush range + Fence
void persist(void* addr, std::size_t len);

// Atomic pointer swap with ordering
void atomic_pointer_swap(std::uint64_t* addr, std::uint64_t new_value, 
                         std::uint64_t* out_old_value);

} // namespace atomic_tree

#endif // ATOMICTREE_PRIMITIVES_H
