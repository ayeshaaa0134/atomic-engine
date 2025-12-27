#include "garbage_collector.h"
#include "B_tree.h"
#include "manager.h"
#include <iostream>
#include <vector>

namespace atomic_tree {

GarbageCollector::GarbageCollector(Manager *manager)
    : manager_(manager), marked_count_(0), freed_count_(0) {}

void GarbageCollector::collect(std::uint64_t root_offset, int max_keys,
                               int leaf_capacity) {
  // 1. Stack for DFS
  std::vector<std::uint64_t> stack;
  if (root_offset != 0) {
    stack.push_back(root_offset);
  }

  // 2. Reachability Map
  std::size_t n_blocks = manager_->block_count();
  std::vector<bool> reachable(n_blocks, false);

  marked_count_ = 0;

  // Phase 1: Mark (Reachability Analysis)
  while (!stack.empty()) {
    std::uint64_t offset = stack.back();
    stack.pop_back();

    std::size_t block_idx = offset / manager_->block_size();
    if (block_idx >= n_blocks)
      continue;

    if (reachable[block_idx])
      continue;
    reachable[block_idx] = true;
    marked_count_++;

    BTreeNode *node = static_cast<BTreeNode *>(manager_->offset_to_ptr(offset));

    if (node->is_leaf) {
      // Use standardized BTree helper for next leaf
      std::uint64_t *next_ptr = BTree::get_leaf_next(node, leaf_capacity);
      if (*next_ptr != 0)
        stack.push_back(*next_ptr);
    } else {
      // Use standardized BTree helpers for internal nodes
      std::uint64_t *children = BTree::get_internal_children(node, max_keys);
      for (int i = 0; i <= node->key_count; ++i) {
        if (children[i] != 0) {
          stack.push_back(children[i]);
        }
      }
    }
  }

  // Phase 2: Sweep (Reclaim Dangling Shadow Nodes)
  freed_count_ = 0;
  std::uint64_t *bitmap = manager_->get_bitmap();

  for (std::size_t i = 0; i < n_blocks; ++i) {
    std::size_t word_idx = i / 64;
    std::size_t bit_idx = i % 64;
    bool allocated = (bitmap[word_idx] >> bit_idx) & 1ULL;

    if (allocated && !reachable[i]) {
      // Leak detected: Allocated but not reachable from root (Dangling Split)
      manager_->free_block(i * manager_->block_size());
      freed_count_++;
    }
  }
}

int GarbageCollector::nodes_marked() const { return marked_count_; }
int GarbageCollector::blocks_freed() const { return freed_count_; }

void GarbageCollector::mark_phase(std::uint64_t) {}
void GarbageCollector::sweep_phase() {}

} // namespace atomic_tree
