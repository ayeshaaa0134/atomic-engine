#include "garbage_collector.h"
#include "manager.h"
#include "B_tree.h" 
#include <vector>
#include <iostream>

namespace atomic_tree {

// Constants matching Manager/main defaults or inferred
static const int GC_MAX_KEYS = 16;
static const int GC_LEAF_CAPACITY = 32;

// Helpers (duplicated from B_tree.cpp logic, ideally in shared internal header)
static inline int* get_keys(BTreeNode* node) {
    return &node->keys;
}

static inline std::uint64_t* get_children(BTreeNode* node) {
    intptr_t keys_end = reinterpret_cast<intptr_t>(&node->keys) + GC_MAX_KEYS * sizeof(int);
    keys_end = (keys_end + 7) & ~7;
    return reinterpret_cast<std::uint64_t*>(keys_end);
}

static inline std::uint64_t get_next_leaf(BTreeNode* node) {
   // Assuming entries start at &node->entries
   // next_leaf is at &entries + capacity
   // Note: BTreeNode struct definition has "next_leaf" member but 
   // B_tree.cpp layout logic puts arrays over the top.
   // We follow B_tree.cpp layout logic:
   // Leaf layout: [entries array of size Capacity] [next_leaf]
   atomic_tree::LeafEntry* entries = &node->entries;
   std::uint64_t* next_ptr = reinterpret_cast<std::uint64_t*>(entries + GC_LEAF_CAPACITY);
   return *next_ptr;
}

GarbageCollector::GarbageCollector(Manager* manager) 
    : manager_(manager), marked_count_(0), freed_count_(0) {}

void GarbageCollector::collect(std::uint64_t root_offset) {
    // 1. Stack for DFS (std::vector<uint64_t>)
    std::vector<std::uint64_t> stack;
    if (root_offset != 0) {
        stack.push_back(root_offset);
    }

    // 2. Reachability Map (std::vector<bool>)
    std::size_t n_blocks = manager_->block_count();
    std::vector<bool> reachable(n_blocks, false);

    marked_count_ = 0;
    
    // Phase 1: Mark
    while (!stack.empty()) {
        std::uint64_t offset = stack.back();
        stack.pop_back();

        std::size_t block_idx = offset / manager_->block_size();
        if (block_idx >= n_blocks) continue;

        if (reachable[block_idx]) continue; // Already marked
        reachable[block_idx] = true;
        marked_count_++;

        BTreeNode* node = static_cast<BTreeNode*>(manager_->offset_to_ptr(offset));
        
        if (node->is_leaf) {
            std::uint64_t next = get_next_leaf(node);
            if (next != 0) stack.push_back(next);
        } else {
            std::uint64_t* children = get_children(node);
            // Internal nodes have key_count+1 children
            for (int i=0; i <= node->key_count; ++i) {
                if (children[i] != 0) {
                    stack.push_back(children[i]);
                }
            }
        }
    }

    // Phase 2: Sweep
    freed_count_ = 0;
    std::uint64_t* bitmap = manager_->get_bitmap();
    
    for (std::size_t i=0; i<n_blocks; ++i) {
        // Check allocator state
        std::size_t word_idx = i / 64;
        std::size_t bit_idx = i % 64;
        bool allocated = (bitmap[word_idx] >> bit_idx) & 1ULL;

        if (allocated && !reachable[i]) {
            // Leak: Allocated but not reachable
            manager_->free_block(i * manager_->block_size());
            freed_count_++;
        }
    }
}

int GarbageCollector::nodes_marked() const { return marked_count_; }
int GarbageCollector::blocks_freed() const { return freed_count_; }

// Unused stubs required by header if we didn't remove them?
// Header declared `mark_phase` and `sweep_phase`.
// We implemented inline in `collect`.
// Let's keep empty stubs or remove declarations in a refactor.
// To satisfy linker if invoked:
void GarbageCollector::mark_phase(std::uint64_t) {}
void GarbageCollector::sweep_phase() {}

} // namespace atomic_tree
