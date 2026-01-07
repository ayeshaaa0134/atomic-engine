#include "B_tree.h"
#include "manager.h"
#include "primitives.h"

#include <cstdint>
#include <cstddef>
#include <cinttypes>
#include <vector>
#include <algorithm>
#include <ranges>
#include <iostream>

namespace atomic_tree {

// Helpers for Node Layout

[[nodiscard]] int *BTree::get_internal_keys(BTreeNode *node) noexcept {
    return reinterpret_cast<int *>(node->data);
}

[[nodiscard]] std::uint64_t *BTree::get_internal_children(BTreeNode *node, int max_keys) noexcept {
    int *keys = get_internal_keys(node);
    auto children_start =
        static_cast<std::intptr_t>(reinterpret_cast<std::uintptr_t>(keys + max_keys));
    children_start = (children_start + 7) & ~std::intptr_t{7}; // Align to 8 bytes
    return reinterpret_cast<std::uint64_t *>(children_start);
}

[[nodiscard]] LeafEntry *BTree::get_leaf_entries(BTreeNode *node) noexcept {
    return reinterpret_cast<LeafEntry *>(node->data);
}

[[nodiscard]] std::uint64_t *BTree::get_leaf_next(BTreeNode *node, int leaf_capacity) noexcept {
    LeafEntry *entries = get_leaf_entries(node);
    auto next_start =
        static_cast<std::intptr_t>(reinterpret_cast<std::uintptr_t>(entries + leaf_capacity));
    next_start = (next_start + 7) & ~std::intptr_t{7}; // Align to 8 bytes
    return reinterpret_cast<std::uint64_t *>(next_start);
}

[[nodiscard]] std::uint32_t BTree::calculate_checksum(BTreeNode *node,
                                                     std::size_t block_size) noexcept {
    const std::uint8_t *byte_ptr =
        reinterpret_cast<const std::uint8_t *>(node);
    std::uint32_t crc = 0xFFFFFFFF;

    for (std::size_t i = 0; i < block_size; ++i) {
        // Skip the checksum field itself (offset 5-8)
        if (i >= 5 && i < 9) [[unlikely]]
            continue;

        crc ^= byte_ptr[i];
        for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }

    return ~crc;
}

void BTree::persist_node(BTreeNode *node) {
    node->checksum = calculate_checksum(node, manager_->block_size());
    persist(node, manager_->block_size());
    manager_->update_persistent_checksum();
}

BTree::BTree(Manager *manager, const BTreeConfig &config)
    : manager_(manager), config_(config) {
    root_offset_ = manager_->get_root_offset();

    if (root_offset_ == 0) [[unlikely]] {
        root_offset_ = manager_->alloc_block();
        BTreeNode *root = offset_to_node(root_offset_);
        root->is_leaf = true;
        root->key_count = 0;

        auto *meta = static_cast<Manager::Metadata *>(manager_->base());
        meta->max_keys = config_.max_keys;
        meta->min_keys = config_.min_keys;
        meta->leaf_capacity = config_.leaf_capacity;
        persist(meta, sizeof(Manager::Metadata));

        std::uint64_t *next = get_leaf_next(root, config_.leaf_capacity);
        *next = 0;

        persist_node(root);
        manager_->set_root_offset(root_offset_);
    } else [[likely]] {
        auto *meta = static_cast<Manager::Metadata *>(manager_->base());
        config_.max_keys = meta->max_keys;
        config_.min_keys = meta->min_keys;
        config_.leaf_capacity = meta->leaf_capacity;
    }
}

[[nodiscard]] BTreeNode *BTree::offset_to_node(std::uint64_t offset) const noexcept {
    if (offset == 0) [[unlikely]]
        return nullptr;

    return static_cast<BTreeNode *>(manager_->offset_to_ptr(offset));
}

void BTree::insert(int key, int value) {
    InsertResult res = insert_internal(root_offset_, key, value);
    if (res.did_split) [[unlikely]] {
        std::uint64_t new_root_offset = manager_->alloc_block();
        BTreeNode *new_root = offset_to_node(new_root_offset);
        new_root->is_leaf = false;
        new_root->key_count = 1;

        int *keys = get_internal_keys(new_root);
        std::uint64_t *children = get_internal_children(new_root, config_.max_keys);

        keys[0] = res.split_key;
        children[0] = root_offset_;
        children[1] = res.new_child_offset;

        persist_node(new_root);
        root_offset_ = new_root_offset;
        manager_->set_root_offset(new_root_offset);
    }
}

BTree::InsertResult BTree::insert_internal(std::uint64_t node_offset, int key, int value) {
    BTreeNode *node = offset_to_node(node_offset);
    if (node->is_leaf) [[likely]] {
        return insert_leaf(node_offset, key, value);
    } else {
        return insert_internal_node(node_offset, key, value);
    }
}

BTree::InsertResult BTree::insert_leaf(std::uint64_t leaf_offset, int key, int value) {
    BTreeNode *leaf = offset_to_node(leaf_offset);
    if (leaf->key_count < config_.leaf_capacity) [[likely]] {
        LeafEntry *entries = get_leaf_entries(leaf);
        int idx = leaf->key_count;
        entries[idx] = LeafEntry{key, value};

        pmem_flush(&entries[idx], sizeof(LeafEntry));
        pmem_fence();

        leaf->key_count++;
        persist_node(leaf);
        return {0, 0, false};
    } else [[unlikely]] {
        InsertResult split_res = split_leaf(leaf_offset);
        if (key >= split_res.split_key) {
            insert_leaf(split_res.new_child_offset, key, value);
        } else {
            insert_leaf(leaf_offset, key, value);
        }

        return split_res;
    }
}

BTree::InsertResult BTree::insert_internal_node(std::uint64_t node_offset, int key, int value) {
    BTreeNode *node = offset_to_node(node_offset);
    int *keys = get_internal_keys(node);
    std::uint64_t *children = get_internal_children(node, config_.max_keys);

    int idx = 0;
    while (idx < node->key_count && key >= keys[idx])
        idx++;

    InsertResult res = insert_internal(children[idx], key, value);
    if (res.did_split) [[unlikely]] {
        if (node->key_count < config_.max_keys) [[likely]] {
            for (int i = node->key_count; i > idx; --i) {
                keys[i] = keys[i - 1];
                children[i + 1] = children[i];
            }

            keys[idx] = res.split_key;
            children[idx + 1] = res.new_child_offset;
            node->key_count++;

            persist_node(node);
            return {0, 0, false};
        } else [[unlikely]] {
            InsertResult my_split = split_internal(node_offset);
            BTreeNode *target;

            if (res.split_key < my_split.split_key) {
                target = node;
            } else {
                target = offset_to_node(my_split.new_child_offset);
            }

            int *t_keys = get_internal_keys(target);
            std::uint64_t *t_children = get_internal_children(target, config_.max_keys);
            int t_count = target->key_count;
            int t_idx = 0;

            while (t_idx < t_count && res.split_key >= t_keys[t_idx])
                t_idx++;

            for (int i = t_count; i > t_idx; --i) {
                t_keys[i] = t_keys[i - 1];
                t_children[i + 1] = t_children[i];
            }

            t_keys[t_idx] = res.split_key;
            t_children[t_idx + 1] = res.new_child_offset;
            target->key_count++;

            persist_node(target);
            return my_split;
        }
    }

    return {0, 0, false};
}

BTree::InsertResult BTree::split_leaf(std::uint64_t old_leaf_offset) {
    BTreeNode *old_leaf = offset_to_node(old_leaf_offset);
    std::uint64_t new_leaf_offset = manager_->alloc_block();
    BTreeNode *new_leaf = offset_to_node(new_leaf_offset);
    new_leaf->is_leaf = true;

    LeafEntry *old_entries = get_leaf_entries(old_leaf);
    LeafEntry *new_entries = get_leaf_entries(new_leaf);

    int total = old_leaf->key_count;
    int mid = total / 2;

    std::vector<LeafEntry> buffer(static_cast<std::size_t>(total));
    for (int i = 0; i < total; ++i) {
        buffer[static_cast<std::size_t>(i)] = old_entries[i];
    }

    std::ranges::sort(buffer, [](const LeafEntry &a, const LeafEntry &b) {
        return a.key < b.key;
    });

    int move_to_new = total - mid;
    for (int i = 0; i < move_to_new; ++i) {
        new_entries[i] = buffer[static_cast<std::size_t>(mid + i)];
    }

    new_leaf->key_count = move_to_new;
    int split_key = buffer[static_cast<std::size_t>(mid)].key;

    std::uint64_t *new_next = get_leaf_next(new_leaf, config_.leaf_capacity);
    std::uint64_t *old_next = get_leaf_next(old_leaf, config_.leaf_capacity);
    *new_next = *old_next;

    persist_node(new_leaf);
    atomic_pointer_swap(old_next, new_leaf_offset, nullptr);
    pmem_fence();

    for (int i = 0; i < mid; ++i) {
        old_entries[i] = buffer[static_cast<std::size_t>(i)];
    }

    old_leaf->key_count = mid;
    persist_node(old_leaf);

    return {split_key, new_leaf_offset, true};
}

BTree::InsertResult BTree::split_internal(std::uint64_t old_node_offset) {
    BTreeNode *old_node = offset_to_node(old_node_offset);
    std::uint64_t new_node_offset = manager_->alloc_block();
    BTreeNode *new_node = offset_to_node(new_node_offset);
    new_node->is_leaf = false;

    int *old_keys = get_internal_keys(old_node);
    std::uint64_t *old_children = get_internal_children(old_node, config_.max_keys);
    int *new_keys = get_internal_keys(new_node);
    std::uint64_t *new_children = get_internal_children(new_node, config_.max_keys);

    int total = old_node->key_count;
    int mid = total / 2;
    int split_key = old_keys[mid];
    int move_count = total - 1 - mid;

    for (int i = 0; i < move_count; ++i) {
        new_keys[i] = old_keys[mid + 1 + i];
        new_children[i] = old_children[mid + 1 + i];
    }

    new_children[move_count] = old_children[total];
    new_node->key_count = move_count;
    persist_node(new_node);

    old_node->key_count = mid;
    persist_node(old_node);

    return {split_key, new_node_offset, true};
}

[[nodiscard]] bool BTree::search(int key, int &out_value) const {
    return search_internal(root_offset_, key, out_value);
}

[[nodiscard]] bool BTree::search_internal(std::uint64_t node_offset, int key,
                                          int &out_value) const {
    BTreeNode *node = offset_to_node(node_offset);
    if (!node) [[unlikely]]
        return false;

    if (node->is_leaf) [[likely]] {
        LeafEntry *entries = get_leaf_entries(node);
        for (int i = 0; i < node->key_count; ++i) {
            if (entries[i].key == key) [[unlikely]] {
                out_value = entries[i].value;
                return true;
            }
        }

        return false;
    } else {
        int *keys = get_internal_keys(node);
        std::uint64_t *children = get_internal_children(node, config_.max_keys);
        int i = 0;
        while (i < node->key_count && key >= keys[i])
            i++;

        return search_internal(children[i], key, out_value);
    }
}

[[nodiscard]] bool BTree::erase(int key) {
    return erase_internal(root_offset_, key);
}

[[nodiscard]] bool BTree::erase_internal(std::uint64_t node_offset, int key) {
    BTreeNode *node = offset_to_node(node_offset);
    if (!node) [[unlikely]]
        return false;

    if (node->is_leaf) [[likely]] {
        return erase_leaf(node_offset, key);
    } else {
        int *keys = get_internal_keys(node);
        std::uint64_t *children = get_internal_children(node, config_.max_keys);

        int i = 0;
        while (i < node->key_count && key >= keys[i])
            i++;

        return erase_internal(children[i], key);
    }
}

[[nodiscard]] bool BTree::erase_leaf(std::uint64_t leaf_offset, int key) {
    BTreeNode *leaf = offset_to_node(leaf_offset);
    LeafEntry *entries = get_leaf_entries(leaf);

    int found_idx = -1;
    for (int i = 0; i < leaf->key_count; ++i) {
        if (entries[i].key == key) [[unlikely]] {
            found_idx = i;
            break;
        }
    }

    if (found_idx == -1) [[unlikely]]
        return false;

    if (found_idx != leaf->key_count - 1) {
        entries[found_idx] = entries[leaf->key_count - 1];
        pmem_flush(&entries[found_idx], sizeof(LeafEntry));
        pmem_fence();
    }

    leaf->key_count--;
    persist_node(leaf);
    return true;
}

[[nodiscard]] std::uint64_t BTree::root_offset() const noexcept {
    return root_offset_;
}

void BTree::print_tree() const {
    std::cout << "Tree print TBD\n";
}

} // namespace atomic_tree
