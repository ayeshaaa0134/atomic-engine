#pragma once
#include "allocator.h"
#include "primitives.h"
#include <cstdint>
#include <vector>


// NV-Tree Constants
const int MAX_ENTRIES = 64; // Tunable for demo

// Persistent Leaf Node
struct NVLeafNode {
  uint64_t next_leaf_offset;
  uint64_t parent_offset; // Backlink for reconstruction
  uint32_t count;
  uint32_t padding;

  struct Entry {
    uint64_t key;
    uint64_t value;
  } entries[MAX_ENTRIES];
};

// Volatile Internal Node
struct NVInternalNode {
  std::vector<uint64_t> keys;
  std::vector<NVInternalNode *> children_dram;
  std::vector<uint64_t> children_offsets; // Persistent reference
};

class NVTree {
public:
  NVTree(Allocator *allocator);

  // API
  void put(uint64_t key, uint64_t value);
  bool get(uint64_t key, uint64_t &value);
  void scan(uint64_t start_key, size_t count,
            std::vector<std::pair<uint64_t, uint64_t>> &results);

  // Debug/Stats
  void reconstruct();

private:
  Allocator *pmem;
  NVInternalNode *root_dram;
  uint64_t root_leaf_offset; // If tree is just one leaf
  bool is_root_leaf;

  // Helpers
  NVLeafNode *get_leaf(uint64_t offset);
  void split_leaf(NVInternalNode *parent, int child_idx, NVLeafNode *leaf,
                  uint64_t leaf_offset);
  NVLeafNode *find_leaf(uint64_t key, NVInternalNode **parent_out,
                        int *index_out, uint64_t *leaf_offset_out);
};
