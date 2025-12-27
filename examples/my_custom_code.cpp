#include "../backend/include/B_tree.h"
#include "../backend/include/garbage_collector.h"
#include "../backend/include/manager.h"
#include <chrono>
#include <iostream>


using namespace atomic_tree;

int main() {
  std::cout
      << R"({"type": "log", "level": "info", "message": "MyAtomicTreeApp initialized"})"
      << std::endl;

  // Create your database
  Manager manager("my_custom_db.dat", 256 * 1024 * 1024, 256, true);

  BTreeConfig config;
  config.max_keys = 16;
  config.min_keys = 8;
  config.leaf_capacity = 32;

  BTree tree(&manager, config);

  std::cout
      << R"({"type": "log", "level": "info", "message": "Inserting 10,000 custom records..."})"
      << std::endl;

  auto start_all = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < 10000; i++) {
    auto start_op = std::chrono::high_resolution_clock::now();
    tree.insert(i, i * 100);
    auto end_op = std::chrono::high_resolution_clock::now();

    // Output metrics for extension visualization (every 500 ops)
    if (i % 500 == 0) {
      double latency =
          std::chrono::duration<double, std::micro>(end_op - start_op).count();
      double throughput =
          (i + 1) * 1000000.0 /
          std::chrono::duration<double, std::micro>(end_op - start_all).count();
      manager.print_telemetry(throughput, latency);
    }
  }

  // Test search
  int value;
  if (tree.search(5000, value)) {
    std::cout << "{\"type\": \"log\", \"level\": \"event\", \"message\": "
                 "\"Found key 5000: "
              << value << "\"}" << std::endl;
  }

  // Run garbage collection
  std::cout << "{\"type\": \"log\", \"level\": \"info\", \"message\": "
               "\"Starting background GC...\"}"
            << std::endl;
  GarbageCollector gc(&manager);
  gc.collect(tree.root_offset());

  std::cout << "{\"type\": \"log\", \"level\": \"event\", \"message\": \"GC "
               "complete. Freed: "
            << gc.blocks_freed() << " blocks\"}" << std::endl;

  std::cout << "{\"type\": \"log\", \"level\": \"info\", \"message\": \"App "
               "execution successful.\"}"
            << std::endl;

  return 0;
}
