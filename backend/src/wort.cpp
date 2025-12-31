#include "wort.h"
#include <cstring>
#include <iostream>

WORT::WORT(Allocator *alloc) : pmem(alloc) {
  root_offset = pmem->alloc_block();
  // Zero out root
  WORTNode *root = (WORTNode *)pmem->get_abs_addr(root_offset);
  memset(root, 0, sizeof(WORTNode));
  Primitives::flush(root);
  Primitives::output_fence();
}

void WORT::put(uint64_t key, uint64_t value) {
  // 8-byte key = 8 levels of Radix-256
  uint64_t curr_offset = root_offset;

  for (int i = 0; i < 8; i++) {
    uint8_t slice = (key >> (56 - i * 8)) & 0xFF; // Extract MSB first
    WORTNode *node = (WORTNode *)pmem->get_abs_addr(curr_offset);

    uint64_t next_offset =
        node->children[slice].offset.load(std::memory_order_acquire);

    if (next_offset == 0) {
      // ALLOCATE NEW NODE
      uint64_t new_node_off =
          pmem->alloc_block(); // Assume block large enough for node256 (needs >
                               // 2KB actually, let's fix Allocator later or
                               // assume simplified)
      WORTNode *new_node = (WORTNode *)pmem->get_abs_addr(new_node_off);
      memset(new_node, 0, sizeof(WORTNode));
      new_node->key_byte = slice;

      // PERSIST NEW NODE CONTENT
      Primitives::flush(new_node);
      Primitives::output_fence();

      // CRITICAL: 8-BYTE ATOMIC UPDATE
      // Point parent to child
      node->children[slice].offset.store(new_node_off,
                                         std::memory_order_release);
      Primitives::record_trace(OpType::ATOMIC_STORE,
                               (uint64_t)&node->children[slice].offset);

      // Persist the pointer? In WORT, failure-atomic relies on this store
      // visibility + flushing.
      Primitives::flush(&node->children[slice].offset);
      Primitives::output_fence();

      curr_offset = new_node_off;
    } else {
      curr_offset = next_offset;
    }
  }

  // Leaf logic (at depth 8, or store value in the last node)
  WORTNode *leaf = (WORTNode *)pmem->get_abs_addr(curr_offset);
  leaf->value = value;
  leaf->is_leaf = true;
  Primitives::flush(&leaf->value);
  Primitives::output_fence();
}

bool WORT::get(uint64_t key, uint64_t &value) {
  uint64_t curr_offset = root_offset;
  for (int i = 0; i < 8; i++) {
    uint8_t slice = (key >> (56 - i * 8)) & 0xFF;
    WORTNode *node = (WORTNode *)pmem->get_abs_addr(curr_offset);
    uint64_t next =
        node->children[slice].offset.load(std::memory_order_acquire);
    if (next == 0)
      return false;
    curr_offset = next;
  }
  WORTNode *leaf = (WORTNode *)pmem->get_abs_addr(curr_offset);
  if (leaf->is_leaf) {
    value = leaf->value;
    return true;
  }
  return false;
}
