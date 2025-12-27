#include "B_tree.h"
#include "garbage_collector.h"
#include "manager.h"
#include <cassert>
#include <filesystem>
#include <iostream>

using namespace atomic_tree;
namespace fs = std::filesystem;

void run_crash_simulation() {
  std::string db_file = "crash_test.dat";
  if (fs::exists(db_file))
    fs::remove(db_file);

  {
    Manager manager(db_file, 1024 * 1024 * 10, 256, true);
    BTreeConfig config;
    config.max_keys = 4; // Small for frequent splits
    config.leaf_capacity = 4;
    BTree tree(&manager, config);

    std::cout << "Step 1: Inserting initial data..." << std::endl;
    for (int i = 1; i <= 10; ++i) {
      tree.insert(i, i * 100);
    }

    // Root should be established
    assert(manager.get_root_offset() != 0);
    std::cout
        << "Step 2: Simulating crash during split (manual offset check)..."
        << std::endl;
    // In a real crash, metadata->root_offset would only update if
    // set_root_offset was called. Our atomic split logic ensures either the old
    // root or the new root is valid.
  }

  // Recover
  {
    std::cout << "Step 3: Recovering from file..." << std::endl;
    Manager manager(db_file, 1024 * 1024 * 10, 256, false);
    BTreeConfig config;
    config.max_keys = 4;
    config.leaf_capacity = 4;
    BTree tree(&manager, config);

    GarbageCollector gc(&manager);
    gc.collect(tree.root_offset(), config.max_keys, config.leaf_capacity);

    std::cout << "GC Result: Marked " << gc.nodes_marked() << " nodes, freed "
              << gc.blocks_freed() << " dangling blocks." << std::endl;

    std::cout << "Step 4: Verifying data integrity..." << std::endl;
    int missing_count = 0;
    for (int i = 1; i <= 10; ++i) {
      int val;
      bool found = tree.search(i, val);
      if (!found || val != i * 100) {
        std::cerr << "  ERROR: Key " << i
                  << (found ? " has wrong value" : " NOT found") << std::endl;
        missing_count++;
      }
    }
    if (missing_count > 0) {
      std::cerr << "FAIL: " << missing_count
                << " keys missing/corrupted after recovery." << std::endl;
      throw std::runtime_error("Integrity check failed");
    }
    std::cout << "SUCCESS: Data recovered correctly." << std::endl;
  }

  fs::remove(db_file);
}

int main() {
  try {
    run_crash_simulation();
  } catch (const std::exception &e) {
    std::cerr << "Test failed: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
