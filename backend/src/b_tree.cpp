#include "b_tree.h"
#include <algorithm>
#include <cstring>
#include <iostream>

NVTree::NVTree(Allocator *alloc)
    : pmem(alloc), root_dram(nullptr), is_root_leaf(true) {
  // Initial State: Single Root Leaf
  root_leaf_offset = pmem->alloc_block(); // Assume block size fits leaf
                                          // (simplification for demo)
  NVLeafNode *leaf = get_leaf(root_leaf_offset);
  memset(leaf, 0, sizeof(NVLeafNode));
  Primitives::flush(leaf);
  Primitives::output_fence();
}

NVLeafNode *NVTree::get_leaf(uint64_t offset) {
  return (NVLeafNode *)pmem->get_abs_addr(offset);
}

bool NVTree::get(uint64_t key, uint64_t &value) {
  NVInternalNode *parent = nullptr;
  int idx = 0;
  uint64_t leaf_off = 0;
  NVLeafNode *leaf =
      find_leaf(key, &parent, &idx, &leaf_off); // Volatile search

  // Log Structured Search (unsorted append)
  // Scan backwards for latest version if duplicates allowed, or just scan all
  for (int i = 0; i < (int)leaf->count; i++) {
    if (leaf->entries[i].key == key) {
      value = leaf->entries[i].value;
      return true;
    }
  }
  return false;
}

// Volatile Tree Traversal
NVLeafNode *NVTree::find_leaf(uint64_t key, NVInternalNode **parent_out,
                              int *index_out, uint64_t *leaf_offset_out) {
  if (is_root_leaf) {
    *parent_out = nullptr;
    *leaf_offset_out = root_leaf_offset;
    return get_leaf(root_leaf_offset);
  }

  NVInternalNode *curr = root_dram;
  while (!curr->children_dram.empty()) {
    // Internal search (sorted)
    auto it = std::lower_bound(curr->keys.begin(), curr->keys.end(), key);
    int idx = std::distance(curr->keys.begin(), it);

    // Internal node logic ... (Skipping full B+Tree implementation for brevity,
    // assuming height 1 for MVP) For MVP, if we only have root leaf, we
    // shouldn't be here. Let's implement basic split logic later.
    break;
  }
  return nullptr; // TODO: Full depth
}

void NVTree::put(uint64_t key, uint64_t value) {
  // 1. Traverse to leaf
  uint64_t leaf_offset =
      is_root_leaf ? root_leaf_offset : 0; // TODO: Real traversal
  NVLeafNode *leaf = get_leaf(leaf_offset);

  // 2. Check Capacity
  if (leaf->count < MAX_ENTRIES) {
    // FAST PATH: Append (Log Structured)
    int pos = leaf->count;
    leaf->entries[pos].key = key;
    leaf->entries[pos].value = value;

    // PERSISTENCE BARRIER
    Primitives::flush(&leaf->entries[pos]);
    Primitives::output_fence();

    leaf->count++; // Atomic update logically
    Primitives::flush(&leaf->count);
    Primitives::output_fence();
  } else {
    // SPLIT PATH: Atomic Split (Shadow Paging)
    // 1. Allocate Shadow
    uint64_t shadow_offset = pmem->alloc_block();
    NVLeafNode *shadow = get_leaf(shadow_offset);

    // 2. Copy + Insert (Sort/Compact optionally, here just Copy)
    // naive split: move half
    memcpy(shadow, leaf, sizeof(NVLeafNode));
    shadow->entries[0].key = key; // Just overwrite for demo mechanics
    shadow->count = 1;

    // 3. Persist Shadow
    Primitives::flush(shadow);
    Primitives::output_fence();

    // 4. ATOMIC SWAP (Update Parent)
    // In full NVTree, we update volatile parent and maybe persist a log.
    // For "Atomic Tree Engine", we trace the atomic pointer update.
    if (is_root_leaf) {
      // New Root logic (Demo)
      root_leaf_offset = shadow_offset;
      // Trace the "swap"
      Primitives::record_trace(OpType::ATOMIC_STORE,
                               (uint64_t)&root_leaf_offset);
    }
  }
}

void NVTree::scan(uint64_t start_key, size_t count,
                  std::vector<std::pair<uint64_t, uint64_t>> &results) {
  // Basic scan
  uint64_t offset = is_root_leaf ? root_leaf_offset : 0;
  while (offset != 0 && results.size() < count) {
    NVLeafNode *leaf = get_leaf(offset);
    for (int i = 0; i < leaf->count; i++) {
      if (leaf->entries[i].key >= start_key) {
        results.push_back({leaf->entries[i].key, leaf->entries[i].value});
      }
    }
    offset = leaf->next_leaf_offset;
  }
}
