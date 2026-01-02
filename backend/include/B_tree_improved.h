#ifndef ATOMIC_TREE_B_TREE_H
#define ATOMIC_TREE_B_TREE_H

#include <cstdint>
#include <cstddef>

namespace atomic_tree {

// Forward declarations
class Manager;

// ========== UPDATED: BTreeNode Structure ==========
struct BTreeNode {
  bool is_leaf;
  int key_count;
  std::uint32_t checksum;
  // NOTE: If you want to add next_node for B-Link tree optimization:
  // std::uint64_t next_node;  // Uncomment for Priority 2 enhancement
  char data[];  // Flexible array for keys/children/entries
};

struct LeafEntry {
  int key;
  int value;
  // NOTE: For tombstone-based deletion (Priority 5 enhancement):
  // bool deleted;
  // char padding[3];
};

struct BTreeConfig {
  int max_keys;
  int min_keys;
  int leaf_capacity;
};

class BTree {
public:
  BTree(Manager *manager, const BTreeConfig &config);

  // Core operations
  void insert(int key, int value);
  bool search(int key, int &out_value) const;
  bool erase(int key);  // ✅ NOW IMPLEMENTED
  
  // Utilities
  std::uint64_t root_offset() const;
  void print_tree() const;

  // Static helpers for node layout
  static int *get_internal_keys(BTreeNode *node);
  static std::uint64_t *get_internal_children(BTreeNode *node, int max_keys);
  static LeafEntry *get_leaf_entries(BTreeNode *node);
  static std::uint64_t *get_leaf_next(BTreeNode *node, int leaf_capacity);

private:
  Manager *manager_;
  BTreeConfig config_;
  std::uint64_t root_offset_;

  struct InsertResult {
    int split_key;
    std::uint64_t new_child_offset;
    bool did_split;
  };

  // Internal operations
  BTreeNode *offset_to_node(std::uint64_t offset) const;
  InsertResult insert_internal(std::uint64_t node_offset, int key, int value);
  InsertResult insert_leaf(std::uint64_t leaf_offset, int key, int value);
  InsertResult insert_internal_node(std::uint64_t node_offset, int key, int value);
  InsertResult split_leaf(std::uint64_t old_leaf_offset);
  InsertResult split_internal(std::uint64_t old_node_offset);
  
  bool search_internal(std::uint64_t node_offset, int key, int &out_value) const;
  
  // ========== NEW: Delete operations ==========
  bool erase_internal(std::uint64_t node_offset, int key);
  bool erase_leaf(std::uint64_t leaf_offset, int key);
  
  // Persistence
  void persist_node(BTreeNode *node);
  static std::uint32_t calculate_checksum(BTreeNode *node, std::size_t block_size);
};

} // namespace atomic_tree

#endif // ATOMIC_TREE_B_TREE_H
