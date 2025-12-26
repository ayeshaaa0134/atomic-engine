#ifndef ATOMICTREE_MANAGER_H
#define ATOMICTREE_MANAGER_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace atomic_tree {

/**
 * Manager: Persistent Memory Allocator with Bitmap
 * 
 * Manages allocation of fixed-size blocks within a persistent memory region.
 * Uses a 1-bit-per-block bitmap for tracking allocation state.
 * 
 * Key Features:
 * - Backed by memory-mapped file ("nvm.dat") for persistence.
 * - Relative pointers (offsets) for crash safety.
 * - O(1) allocation using TZCNT instruction.
 */
class Manager {
public:
    // Initialize manager over a persistent file
    // @param filename: Path to the backing file (e.g. "nvm.dat")
    // @param region_size: Total size in bytes (e.g. 1GB)
    // @param block_size: Fixed block size (e.g. 256 bytes)
    // @param create_new: If true, overwrites existing file. If false, opens existing.
    Manager(const std::string& filename, std::size_t region_size, std::size_t block_size, bool create_new = false);
    
    ~Manager();

    // Allocate a single block, return offset from base
    std::uint64_t alloc_block();

    // Free a previously allocated block at given offset
    void free_block(std::uint64_t offset);

    // Convert offset back to raw pointer for in-memory access
    void* offset_to_ptr(std::uint64_t offset);

    void* base() const;
    std::size_t region_size() const;
    std::size_t block_size() const;
    std::size_t block_count() const;

    std::uint64_t* get_bitmap();

private:
    void*        base_;
    std::size_t  region_size_;
    std::size_t  block_size_;
    std::size_t  block_count_;
    
    // Windows Handles
    void*        file_handle_;
    void*        map_handle_;

    std::uint64_t* bitmap_;
    std::size_t    bitmap_size_words_;
};

} // namespace atomic_tree

#endif // ATOMICTREE_MANAGER_H
