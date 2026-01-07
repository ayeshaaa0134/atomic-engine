#ifndef ATOMIC_TREE_BTREE_H
#define ATOMIC_TREE_BTREE_H

#include <cstddef>
#include <cstdint>

namespace atomic_tree {

class Manager;

struct LeafEntry {
    int key;
    int value;
};

struct BTreeNode {
    bool         is_leaf;      // 1 byte
    std::uint8_t _pad1[3];     // pad to 4
    std::uint32_t key_count;   // 4 bytes
    std::uint32_t checksum;    // 4 bytes (bytes 8–11)
    std::uint8_t  _pad2[4];    // pad so data[] is 8‑byte aligned
    std::uint8_t  data[];      // flexible array for keys/children or leaf entries
};

struct BTreeConfig {
    int max_keys;
    int min_keys;
    int leaf_capacity;
};

class BTree {
public:
    struct InsertResult {
        int           split_key;
        std::uint64_t new_child_offset;
        bool          did_split;
    };

    BTree(Manager *manager, const BTreeConfig &config);

    void insert(int key, int value);

    [[nodiscard]] bool search(int key, int &out_value) const;

    [[nodiscard]] bool erase(int key);

    [[nodiscard]] std::uint64_t root_offset() const noexcept;

    void print_tree() const;

    // layout helpers used by GC and others
    [[nodiscard]] static int *get_internal_keys(BTreeNode *node) noexcept;
    [[nodiscard]] static std::uint64_t *get_internal_children(BTreeNode *node,
                                                              int max_keys) noexcept;
    [[nodiscard]] static LeafEntry *get_leaf_entries(BTreeNode *node) noexcept;
    [[nodiscard]] static std::uint64_t *get_leaf_next(BTreeNode *node,
                                                      int leaf_capacity) noexcept;

private:
    Manager     *manager_;
    BTreeConfig  config_;
    std::uint64_t root_offset_;

    [[nodiscard]] static std::uint32_t calculate_checksum(BTreeNode *node,
                                                          std::size_t block_size) noexcept;
    void persist_node(BTreeNode *node);

    [[nodiscard]] BTreeNode *offset_to_node(std::uint64_t offset) const noexcept;

    InsertResult insert_internal(std::uint64_t node_offset, int key, int value);
    InsertResult insert_leaf(std::uint64_t leaf_offset, int key, int value);
    InsertResult insert_internal_node(std::uint64_t node_offset, int key, int value);
    InsertResult split_leaf(std::uint64_t old_leaf_offset);
    InsertResult split_internal(std::uint64_t old_node_offset);

    [[nodiscard]] bool search_internal(std::uint64_t node_offset,
                                       int key,
                                       int &out_value) const;

    [[nodiscard]] bool erase_internal(std::uint64_t node_offset, int key);
    [[nodiscard]] bool erase_leaf(std::uint64_t leaf_offset, int key);
};

} // namespace atomic_tree

#endif // ATOMIC_TREE_BTREE_H
