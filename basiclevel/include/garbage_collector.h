#ifndef ATOMIC_TREE_GARBAGE_COLLECTOR_H
#define ATOMIC_TREE_GARBAGE_COLLECTOR_H

#include <cstdint>

namespace atomic_tree {

class Manager;
struct BTreeNode;
class BTree;

class GarbageCollector {
public:
    explicit GarbageCollector(Manager *manager);

    void collect(std::uint64_t root_offset,
                 int max_keys,
                 int leaf_capacity);

    [[nodiscard]] int nodes_marked() const noexcept;
    [[nodiscard]] int blocks_freed() const noexcept;

    void mark_phase(std::uint64_t root_offset);
    void sweep_phase();

private:
    Manager *manager_;
    int      marked_count_;
    int      freed_count_;
};

} // namespace atomic_tree

#endif // ATOMIC_TREE_GARBAGE_COLLECTOR_H
