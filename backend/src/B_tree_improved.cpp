#include "B_tree.h"
#include "manager.h"
#include "primitives.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

namespace atomic_tree {

// Helpers for Node Layout
int *BTree::get_internal_keys(BTreeNode *node) {
  return reinterpret_cast<int *>(node->data);
}

std::uint64_t *BTree::get_internal_children(BTreeNode *node, int max_keys) {
  int *keys = get_internal_keys(node);
  intptr_t children_start = reinterpret_cast<intptr_t>(keys + max_keys);
  children_start = (children_start + 7) & ~7; // Align to 8 bytes
  return reinterpret_cast<std::uint64_t *>(children_start);
}

LeafEntry *BTree::get_leaf_entries(BTreeNode *node) {
  return reinterpret_cast<LeafEntry *>(node->data);
}

std::uint64_t *BTree::get_leaf_next(BTreeNode *node, int leaf_capacity) {
  LeafEntry *entries = get_leaf_entries(node);
  intptr_t next_start = reinterpret_cast<intptr_t>(entries + leaf_capacity);
  next_start = (next_start + 7) & ~7; // Align to 8 bytes
  return reinterpret_cast<std::uint64_t *>(next_start);
}

std::uint32_t BTree::calculate_checksum(BTreeNode *node,
                                        std::size_t block_size) {
  const std::uint8_t *byte_ptr = reinterpret_cast<const std::uint8_t *>(node);
  std::uint32_t crc = 0xFFFFFFFF;
  for (std::size_t i = 0; i < block_size; ++i) {
    // Skip the checksum field itself (offset 5-8)
    if (i >= 5 && i < 9)
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
  // Update global persistent checksum after node changes
  manager_->update_persistent_checksum();
}

BTree::BTree(Manager *manager, const BTreeConfig &config)
    : manager_(manager), config_(config) {

  // Load root from manager
  root_offset_ = manager_->get_root_offset();

  if (root_offset_ == 0) {
    // New Tree: Allocate Root
    root_offset_ = manager_->alloc_block();
    BTreeNode *root = offset_to_node(root_offset_);

    root->is_leaf = true;
    root->key_count = 0;

    // Sync Config to Manager
    auto *meta = static_cast<Manager::Metadata *>(manager_->base());
    meta->max_keys = config_.max_keys;
    meta->min_keys = config_.min_keys;
    meta->leaf_capacity = config_.leaf_capacity;
    persist(meta, sizeof(Manager::Metadata));

    // Initialize next_leaf using helper
    std::uint64_t *next = get_leaf_next(root, config_.leaf_capacity);
    *next = 0;

    persist_node(root);
    manager_->set_root_offset(root_offset_);
  } else {
    // Load existing config from manager
    auto *meta = static_cast<Manager::Metadata *>(manager_->base());
    config_.max_keys = meta->max_keys;
    config_.min_keys = meta->min_keys;
    config_.leaf_capacity = meta->leaf_capacity;
  }
}

BTreeNode *BTree::offset_to_node(std::uint64_t offset) const {
  if (offset == 0)
    return nullptr;
  return static_cast<BTreeNode *>(manager_->offset_to_ptr(offset));
}

void BTree::insert(int key, int value) {
  InsertResult res = insert_internal(root_offset_, key, value);

  if (res.did_split) {
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

    // Update root (Volatile in this object AND persistent in the Manager
    // Metadata)
    root_offset_ = new_root_offset;
    manager_->set_root_offset(new_root_offset);
  }
}

BTree::InsertResult BTree::insert_internal(std::uint64_t node_offset, int key,
                                           int value) {
  BTreeNode *node = offset_to_node(node_offset);
  if (node->is_leaf) {
    return insert_leaf(node_offset, key, value);
  } else {
    return insert_internal_node(node_offset, key, value);
  }
}

BTree::InsertResult BTree::insert_leaf(std::uint64_t leaf_offset, int key,
                                       int value) {
  BTreeNode *leaf = offset_to_node(leaf_offset);

  if (leaf->key_count < config_.leaf_capacity) {
    LeafEntry *entries = get_leaf_entries(leaf);
    int idx = leaf->key_count;
    entries[idx] = {key, value};

    // 1. Flush New Entry (NV-Tree Style)
    pmem_flush(&entries[idx], sizeof(LeafEntry));

    // 2. Fence to ensure entry is durable before count update
    pmem_fence();

    // 3. Update Count and Flush (Atomic commit of entry)
    leaf->key_count++;
    persist_node(leaf);

    return {0, 0, false};
  } else {
    // Shadow Split pattern
    InsertResult split_res = split_leaf(leaf_offset);
    if (key >= split_res.split_key) {
      insert_leaf(split_res.new_child_offset, key, value);
    } else {
      insert_leaf(leaf_offset, key, value);
    }
    return split_res;
  }
}

BTree::InsertResult BTree::insert_internal_node(std::uint64_t node_offset,
                                                int key, int value) {
  BTreeNode *node = offset_to_node(node_offset);
  int *keys = get_internal_keys(node);
  std::uint64_t *children = get_internal_children(node, config_.max_keys);

  int idx = 0;
  while (idx < node->key_count && key >= keys[idx])
    idx++;

  InsertResult res = insert_internal(children[idx], key, value);

  if (res.did_split) {
    if (node->key_count < config_.max_keys) {
      // Shadow Update inside the node (Atomic Commit Pattern):
      // 1. Shift existing keys/children in the "garbage" area beyond
      // key_count
      for (int i = node->key_count; i > idx; i--) {
        keys[i] = keys[i - 1];
        children[i + 1] = children[i];
      }

      // 2. Insert new key/child from split
      keys[idx] = res.split_key;
      children[idx + 1] = res.new_child_offset;

      // 3. Flush the changed portion of the node
      node->key_count++;
      persist_node(node);

      return {0, 0, false};
    } else {
      // Internal Node is full -> Split recursively
      InsertResult my_split = split_internal(node_offset);
      BTreeNode *target;
      if (res.split_key < my_split.split_key) {
        target = node;
      } else {
        target = offset_to_node(my_split.new_child_offset);
      }

      // Insert into target (guaranteed space due to split)
      int *t_keys = get_internal_keys(target);
      std::uint64_t *t_children =
          get_internal_children(target, config_.max_keys);
      int t_count = target->key_count;

      int t_idx = 0;
      while (t_idx < t_count && res.split_key >= t_keys[t_idx])
        t_idx++;

      for (int i = t_count; i > t_idx; i--) {
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

  // 1. Allocate Shadow Node (New Right Sibling)
  std::uint64_t new_leaf_offset = manager_->alloc_block();
  BTreeNode *new_leaf = offset_to_node(new_leaf_offset);

  new_leaf->is_leaf = true;

  LeafEntry *old_entries = get_leaf_entries(old_leaf);
  LeafEntry *new_entries = get_leaf_entries(new_leaf);

  int total = old_leaf->key_count;
  int mid = total / 2;

  // Buffer sort to find the split point (Leaves are unsorted in NV-Tree)
  std::vector<LeafEntry> buffer(total);
  for (int i = 0; i < total; ++i)
    buffer[i] = old_entries[i];
  std::sort(
      buffer.begin(), buffer.end(),
      [](const LeafEntry &a, const LeafEntry &b) { return a.key < b.key; });

  // 2. Populate Shadow Node with larger half
  int move_to_new = total - mid;
  for (int i = 0; i < move_to_new; ++i) {
    new_entries[i] = buffer[mid + i];
  }
  new_leaf->key_count = move_to_new;

  int split_key = buffer[mid].key;

  // 3. Leaf Chaining (Consistency Step 1): New->Next = Old->Next
  std::uint64_t *new_next = get_leaf_next(new_leaf, config_.leaf_capacity);
  std::uint64_t *old_next = get_leaf_next(old_leaf, config_.leaf_capacity);
  *new_next = *old_next;

  // 4. Flush the entire Shadow Node
  persist_node(new_leaf);

  // 5. Atomic Pointer Update (Consistency Step 2): Old->Next = New
  // This makes the new node reachable via the leaf chain even before parent
  // update
  atomic_pointer_swap(old_next, new_leaf_offset, nullptr);
  pmem_fence();

  // 6. Shrink Old Leaf in-place (Consistent because key_count update is
  // atomic)
  for (int i = 0; i < mid; ++i)
    old_entries[i] = buffer[i];
  old_leaf->key_count = mid;
  persist_node(old_leaf);

  return {split_key, new_leaf_offset, true};
}

BTree::InsertResult BTree::split_internal(std::uint64_t old_node_offset) {
  BTreeNode *old_node = offset_to_node(old_node_offset);

  // 1. Allocate Shadow Node (New Right Sibling)
  std::uint64_t new_node_offset = manager_->alloc_block();
  BTreeNode *new_node = offset_to_node(new_node_offset);

  new_node->is_leaf = false;

  int *old_keys = get_internal_keys(old_node);
  std::uint64_t *old_children =
      get_internal_children(old_node, config_.max_keys);

  int *new_keys = get_internal_keys(new_node);
  std::uint64_t *new_children =
      get_internal_children(new_node, config_.max_keys);

  int total = old_node->key_count;
  int mid = total / 2;
  int split_key = old_keys[mid];

  // 2. Populate Shadow Node with larger half
  int move_count = total - 1 - mid; // One key moves up to parent
  for (int i = 0; i < move_count; ++i) {
    new_keys[i] = old_keys[mid + 1 + i];
    new_children[i] = old_children[mid + 1 + i];
  }
  // Last child of the new node
  new_children[move_count] = old_children[total];
  new_node->key_count = move_count;

  // 3. Flush the complete shadow node first
  persist_node(new_node);

  // 4. Shrink old node in-place
  old_node->key_count = mid;
  persist_node(old_node);

  return {split_key, new_node_offset, true};
}

bool BTree::search(int key, int &out_value) const {
  return search_internal(root_offset_, key, out_value);
}

bool BTree::search_internal(std::uint64_t node_offset, int key,
                            int &out_value) const {
  BTreeNode *node = offset_to_node(node_offset);
  if (!node)
    return false;

  if (node->is_leaf) {
    LeafEntry *entries = get_leaf_entries(node);
    // Linear scan
    for (int i = 0; i < node->key_count; ++i) {
      if (entries[i].key == key) {
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

// ========== NEW: DELETE IMPLEMENTATION ==========

bool BTree::erase(int key) { 
  return erase_internal(root_offset_, key); 
}

bool BTree::erase_internal(std::uint64_t node_offset, int key) {
  BTreeNode *node = offset_to_node(node_offset);
  if (!node)
    return false;

  if (node->is_leaf) {
    return erase_leaf(node_offset, key);
  } else {
    // Navigate to correct child
    int *keys = get_internal_keys(node);
    std::uint64_t *children = get_internal_children(node, config_.max_keys);
    
    int i = 0;
    while (i < node->key_count && key >= keys[i])
      i++;
    
    return erase_internal(children[i], key);
  }
}

bool BTree::erase_leaf(std::uint64_t leaf_offset, int key) {
  BTreeNode *leaf = offset_to_node(leaf_offset);
  LeafEntry *entries = get_leaf_entries(leaf);
  
  // Find the key (linear scan since unsorted)
  int found_idx = -1;
  for (int i = 0; i < leaf->key_count; ++i) {
    if (entries[i].key == key) {
      found_idx = i;
      break;
    }
  }
  
  if (found_idx == -1)
    return false;  // Key not found
  
  // NV-Tree Style Lazy Deletion:
  // Step 1: If not last entry, swap with last entry
  if (found_idx != leaf->key_count - 1) {
    entries[found_idx] = entries[leaf->key_count - 1];
    pmem_flush(&entries[found_idx], sizeof(LeafEntry));
    pmem_fence();
  }
  
  // Step 2: Atomically decrease count (this is the commit point)
  leaf->key_count--;
  persist_node(leaf);
  
  return true;
}

// ========== END DELETE IMPLEMENTATION ==========

std::uint64_t BTree::root_offset() const { return root_offset_; }
void BTree::print_tree() const { std::cout << "Tree print TBD\n"; }

} // namespace atomic_tree
