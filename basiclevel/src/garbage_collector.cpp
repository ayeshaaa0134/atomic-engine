#include "garbage_collector.h"
#include "B_tree.h"
#include "manager.h"

#include <cstdint>
#include <cstddef>
#include <vector>
#include <bit>
#include <format>
#include <iostream>

namespace atomic_tree {

GarbageCollector::GarbageCollector(Manager *manager)
    : manager_(manager), marked_count_(0), freed_count_(0) {}

void GarbageCollector::collect(std::uint64_t root_offset, int max_keys, int leaf_capacity) {
    std::vector<std::uint64_t> stack;

    if (root_offset != 0) [[likely]] {
        stack.push_back(root_offset);
    }

    std::size_t n_blocks = manager_->block_count();
    std::vector<bool> reachable(n_blocks, false);

    marked_count_ = 0;

    while (!stack.empty()) {
        std::uint64_t offset = stack.back();
        stack.pop_back();

        std::size_t block_idx = offset / manager_->block_size();
        if (block_idx >= n_blocks) [[unlikely]]
            continue;

        if (reachable[block_idx]) [[unlikely]]
            continue;

        reachable[block_idx] = true;
        marked_count_++;

        BTreeNode *node =
            static_cast<BTreeNode *>(manager_->offset_to_ptr(offset));

        if (node->is_leaf) [[likely]] {
            std::uint64_t *next_ptr = BTree::get_leaf_next(node, leaf_capacity);
            if (*next_ptr != 0) [[unlikely]]
                stack.push_back(*next_ptr);
        } else {
            std::uint64_t *children = BTree::get_internal_children(node, max_keys);
            for (int i = 0; i <= node->key_count; ++i) {
                if (children[i] != 0) [[likely]] {
                    stack.push_back(children[i]);
                }
            }
        }
    }

    freed_count_ = 0;

    std::uint64_t *bitmap = manager_->get_bitmap();
    std::size_t bitmap_words = (n_blocks + 63) / 64;

    for (std::size_t i = 0; i < bitmap_words; ++i) {
        std::uint64_t word = bitmap[i];
        if (word == 0) [[unlikely]]
            continue;

        for (std::size_t bit = 0; bit < 64; ++bit) {
            std::size_t block_idx = i * 64 + bit;
            if (block_idx >= n_blocks) [[unlikely]]
                break;

            bool allocated = std::has_single_bit(word & (1ULL << bit));
            if (allocated && !reachable[block_idx]) [[unlikely]] {
                manager_->free_block(block_idx * manager_->block_size());
                freed_count_++;
            }
        }
    }

    if (freed_count_ > 0) [[unlikely]] {
        manager_->update_persistent_checksum();
        std::cout << std::format(
                         R"({{"type": "gc_log", "marked": {}, "freed": {}, "fragmentation": "{:.2f}%"}})",
                         marked_count_, freed_count_,
                         (freed_count_ * 100.0 / (marked_count_ + freed_count_)))
                  << std::endl;
    }
}

[[nodiscard]] int GarbageCollector::nodes_marked() const noexcept {
    return marked_count_;
}

[[nodiscard]] int GarbageCollector::blocks_freed() const noexcept {
    return freed_count_;
}

void GarbageCollector::mark_phase(std::uint64_t) {}

void GarbageCollector::sweep_phase() {}

} // namespace atomic_tree
