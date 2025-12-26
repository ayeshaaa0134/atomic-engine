#ifndef ATOMICTREE_GARBAGE_COLLECTOR_H
#define ATOMICTREE_GARBAGE_COLLECTOR_H

#include <cstdint>

namespace atomic_tree {

class Manager;

/**
 * Garbage Collector: Mark-and-Sweep for Crash Recovery
 * 
 * Purpose: Clean up shadow nodes and leaked blocks from crashed transactions
 * 
 * Two-Phase Algorithm (from your slides):
 * Phase 1 (Mark): DFS traversal from root, mark all reachable nodes in bitmap
 * Phase 2 (Sweep): Compare reachability bitmap with allocator bitmap
 *                  - If allocated but not marked → shadow node from failed split
 *                  - Free these blocks to prevent memory leaks
 * 
 * Runs automatically on startup to ensure consistent state
 */
class GarbageCollector {
public:
    explicit GarbageCollector(Manager* manager);

    // Run full GC starting from B-tree root
    void collect(std::uint64_t root_offset);

    // Statistics
    int nodes_marked() const;
    int blocks_freed() const;

private:
    Manager* manager_;
    int marked_count_;
    int freed_count_;

    // Phase 1: Mark all reachable nodes via DFS
    void mark_phase(std::uint64_t node_offset);

    // Phase 2: Sweep unmarked blocks and free them
    void sweep_phase();
};

} // namespace atomic_tree

#endif // ATOMICTREE_GARBAGE_COLLECTOR_H
