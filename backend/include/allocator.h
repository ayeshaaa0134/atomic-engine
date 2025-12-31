#pragma once
#include <cstdint>
#include <string>
#include <vector>

class Allocator {
public:
  static const uint64_t BLOCK_SIZE = 64;                // Cache Line
  static const uint64_t POOL_SIZE = 1024 * 1024 * 1024; // 1GB Simulated PM

  Allocator(const std::string &filename);
  ~Allocator();

  // Returns relative offset from base
  uint64_t alloc_block();
  void free_block(uint64_t offset);

  // Address translation
  void *get_abs_addr(uint64_t offset);
  uint64_t get_rel_offset(void *addr);

  // Metrics
  size_t get_used_blocks() const { return used_blocks_count; }

private:
  void *base_addr;
  void *bitmap_addr;
  uint64_t total_blocks;
  uint64_t used_blocks_count;
  void *file_handle;
  void *map_handle;

  void init_bitmap();
  int find_free_bit();
  void set_bit(int index);
  void clear_bit(int index);
};
