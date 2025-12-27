#include "B_tree.h"
#include "garbage_collector.h"
#include "manager.h"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
  std::cout << "AtomicTree Engine - CXL Storage Demo" << std::endl;

  try {
    // Create/Open 1GB file "nvm.dat"
    // Block size 256 bytes
    size_t region_size = 1024 * 1024 * 1024; // 1 GB
    std::string db_file = "nvm.dat";

    std::cout << "Initializing Persistent Memory Manager (" << db_file << ")..."
              << std::endl;
    // true = create new (reset) for this demo
    atomic_tree::Manager manager(db_file, region_size, 512, true);

    atomic_tree::BTreeConfig config;
    config.max_keys = 16;
    config.min_keys = 8;
    config.leaf_capacity = 32;

    std::cout << "Initializing B+ Tree..." << std::endl;
    atomic_tree::BTree tree(&manager, config);

    // Run GC on startup to clean up any orphaned shadow nodes from a previous
    // crash
    atomic_tree::GarbageCollector gc(&manager);
    gc.collect(tree.root_offset(), config.max_keys, config.leaf_capacity);
    if (gc.blocks_freed() > 0) {
      std::cout << "{\"type\": \"log\", \"level\": \"warn\", \"message\": "
                   "\"Recovered "
                << gc.blocks_freed() << " dangling blocks from crash.\"}"
                << std::endl;
    }

    std::cout << "Root offset: " << tree.root_offset() << std::endl;

    // Simple Insert Test
    std::cout << "{\"type\": \"log\", \"level\": \"info\", \"message\": "
                 "\"Inserting 1000 keys...\"}"
              << std::endl;
    for (int i = 0; i < 1000; i++) {
      auto start = std::chrono::high_resolution_clock::now();
      tree.insert(i, i * 10);
      auto end = std::chrono::high_resolution_clock::now();

      if (i % 50 == 0) {
        double latency =
            std::chrono::duration<double, std::micro>(end - start).count();
        manager.print_telemetry(5000.0, latency); // 5k ops/sec nominal rate
      }
    }
    std::cout << "{\"type\": \"log\", \"level\": \"info\", \"message\": "
                 "\"Insertion complete.\"}"
              << std::endl;

    // Search Test
    int val;
    if (tree.search(500, val)) {
      std::cout << "{\"type\": \"log\", \"level\": \"info\", \"message\": "
                   "\"Found key 500: "
                << val << "\"}" << std::endl;
    } else {
      std::cout << "{\"type\": \"log\", \"level\": \"warn\", \"message\": "
                   "\"Key 500 NOT found!\"}"
                << std::endl;
    }

    // Garbage Collection Test (Manual Trigger)
    std::cout << "Running Garbage Collector..." << std::endl;
    gc.collect(tree.root_offset(), config.max_keys, config.leaf_capacity);
    std::cout << "GC Done. Marked: " << gc.nodes_marked()
              << ", Freed: " << gc.blocks_freed() << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
