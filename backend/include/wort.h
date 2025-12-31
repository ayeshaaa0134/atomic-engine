#pragma once
#include "allocator.h"
#include "primitives.h"
#include <atomic>

// 8-byte atomic pointer wrapper for WORT children
struct AtomicChildPtr {
  std::atomic<uint64_t> offset;
};

struct WORTNode {
  uint8_t type;     // 0 = Node256 (implied for MVP)
  uint8_t key_byte; // The byte leading to this node (optimization)
  uint64_t value;   // If leaf
  bool is_leaf;

  // Simple Radix-256
  AtomicChildPtr children[256];
};

class WORT {
public:
  WORT(Allocator *alloc);
  void put(uint64_t key, uint64_t value);
  bool get(uint64_t key, uint64_t &value);

private:
  Allocator *pmem;
  uint64_t root_offset; // Local root pointer (volatile) but points to PM

  // Helpers
  uint64_t get_or_alloc_node(uint64_t parent_offset, int byte_idx);
};
