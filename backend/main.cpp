#include <iostream>
#include "manager.h"
#include "B_tree.h"
#include "garbage_collector.h"

int main() {
    std::cout << "AtomicTree Engine - CXL Storage Demo" << std::endl;
    
    try {
        // Create/Open 1GB file "nvm.dat"
        // Block size 256 bytes
        size_t region_size = 1024 * 1024 * 1024; // 1 GB
        std::string db_file = "nvm.dat";
        
        std::cout << "Initializing Persistent Memory Manager (" << db_file << ")..." << std::endl;
        // true = create new (reset) for this demo
        atomic_tree::Manager manager(db_file, region_size, 256, true); 
        
        atomic_tree::BTreeConfig config;
        config.max_keys = 16;
        config.min_keys = 8;
        config.leaf_capacity = 32;

        std::cout << "Initializing B-Tree..." << std::endl;
        atomic_tree::BTree tree(&manager, config);
        
        std::cout << "Root offset: " << tree.root_offset() << std::endl;

        // Simple Insert Test
        std::cout << "Inserting 1000 keys..." << std::endl;
        for (int i=0; i<1000; i++) {
            tree.insert(i, i*10);
        }
        std::cout << "Insertion complete." << std::endl;
        
        // Search Test
        int val;
        if (tree.search(500, val)) {
            std::cout << "Found key 500: " << val << std::endl;
        } else {
            std::cout << "Key 500 NOT found!" << std::endl;
        }

        // Garbage Collection Test (Manual Trigger)
        std::cout << "Running Garbage Collector..." << std::endl;
        atomic_tree::GarbageCollector gc(&manager);
        gc.collect(tree.root_offset());
        std::cout << "GC Done. Marked: " << gc.nodes_marked() << ", Freed: " << gc.blocks_freed() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
