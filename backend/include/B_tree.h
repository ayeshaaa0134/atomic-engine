#ifndef ATOMICTREE_B_TREE_H
#define ATOMICTREE_B_TREE_H

#include <cstdint>
#include <vector>

namespace atomic_tree {

class Manager;

/**
 * On-disk/In-NVM Data Structures
 *
 * NV-Tree distinguishes between:
 * - Internal nodes: Sorted keys and child offsets (like standard B+ Tree)
 * - Leaf nodes: UNSORTED key-value pairs (append-only for efficiency)
 */

// Configuration parameters for B+ Tree tuning
struct BTreeConfig {
  int max_keys;      // Maximum keys per internal node
  int min_keys;      // Minimum keys per node (except root)
  int leaf_capacity; // Number of entry slots in a leaf
};

// Leaf Entry: Stores key-value pair in a leaf node
struct LeafEntry {
  int key;
  int value;
};

struct BTreeNode {
  // Metadata (8-byte aligned for atomic updates)
  bool is_leaf;  // 1 byte
  int key_count; // 4 bytes: number of keys in this node
  int padding;   // 3 bytes: alignment to 8-byte boundary

  // Body: Variable-sized data depending on is_leaf.
  // We use byte-level offsets to access keys, children, and entries
  // to support heterogeneous node sizes and professional memory safety.
  char data[1]; // Flexible data start
};

/**
 * B+ Tree Index with Atomic Splits (NV-Tree + WORT)
 *
 * Core operations follow the pattern:
 * 1. Shadow Allocation: Create new node in NVM
 * 2. Populate: Copy/organize data into new node
 * 3. Flush: Call pmem_flush on new node data
 * 4. Fence: Call pmem_fence to ensure durability
 * 5. Atomic Swap: Update parent pointer with single 8-byte write
 *
 * At any power failure point, tree remains consistent because:
 * - Old structure still valid if swap hasn't occurred
 * - New structure valid if swap has occurred (pointed to)
 */
class BTree {
public:
  // Initialize B+ Tree over persistent memory via manager
  BTree(Manager *manager, const BTreeConfig &config);

  // Core API
  void insert(int key, int value);            // Insert key-value pair
  bool search(int key, int &out_value) const; // Search for key
  bool erase(int key);                        // Delete key (optional)

  // Recovery and diagnostic
  std::uint64_t root_offset() const; // Get root for GC and recovery
  void print_tree() const;           // Debug: print tree structure

  // Help to get children and keys based on node type
  static int *get_internal_keys(BTreeNode *node);
  static std::uint64_t *get_internal_children(BTreeNode *node, int max_keys);
  static LeafEntry *get_leaf_entries(BTreeNode *node);
  static std::uint64_t *get_leaf_next(BTreeNode *node, int leaf_capacity);

  // Return type for internal splits
  struct InsertResult {
    int split_key;
    std::uint64_t new_child_offset;
    bool did_split;
  };

private:
  Manager *manager_;
  BTreeConfig config_;
  std::uint64_t root_offset_;

  // Helper to convert offset to pointer
  BTreeNode *offset_to_node(std::uint64_t offset) const;

  // Internal recursive insertion with split detection
  InsertResult insert_internal(std::uint64_t node_offset, int key, int value);
  InsertResult insert_leaf(std::uint64_t leaf_offset, int key, int value);
  InsertResult insert_internal_node(std::uint64_t node_offset, int key,
                                    int value);

  InsertResult split_leaf(std::uint64_t old_leaf_offset);
  InsertResult split_internal(std::uint64_t old_node_offset);

  // Search helper
  bool search_internal(std::uint64_t node_offset, int key,
                       int &out_value) const;
};

} // namespace atomic_tree

#endif // ATOMICTREE_B_TREE_H
