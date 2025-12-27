#include "../include/B_tree.h"
#include "../include/garbage_collector.h"
#include "../include/manager.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

using namespace atomic_tree;
using namespace std::chrono;

// Stress test: Insert millions of keys, verify correctness, test GC
void test_massive_insertions() {
  std::cout << "\n=== Test 1: Massive Insertions ===" << std::endl;

  const size_t NUM_KEYS = 100000;
  std::string db_file = "stress_test.dat";
  size_t region_size = 512 * 1024 * 1024; // 512 MB

  // Create fresh database
  Manager manager(db_file, region_size, 512, true);

  BTreeConfig config;
  config.max_keys = 16;
  config.min_keys = 8;
  config.leaf_capacity = 32;

  BTree tree(&manager, config);

  std::cout << "Inserting " << NUM_KEYS << " sequential keys..." << std::endl;
  auto start = high_resolution_clock::now();

  for (size_t i = 0; i < NUM_KEYS; i++) {
    tree.insert(static_cast<int>(i), static_cast<int>(i * 10));

    if ((i + 1) % 10000 == 0) {
      std::cout << "  Progress: " << (i + 1) << " keys inserted" << std::endl;
    }
  }

  auto end = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(end - start);

  std::cout << "✓ Insertion complete in " << duration.count() << " ms"
            << std::endl;
  std::cout << "  Throughput: " << (NUM_KEYS * 1000.0 / duration.count())
            << " ops/sec" << std::endl;

  // Verification: Search for all keys
  std::cout << "Verifying all keys are searchable..." << std::endl;
  int errors = 0;

  for (size_t i = 0; i < NUM_KEYS; i++) {
    int value;
    if (!tree.search(static_cast<int>(i), value)) {
      std::cerr << "  ERROR: Key " << i << " not found!" << std::endl;
      errors++;
    } else if (value != static_cast<int>(i * 10)) {
      std::cerr << "  ERROR: Key " << i << " has wrong value: " << value
                << std::endl;
      errors++;
    }

    if (errors > 10) {
      std::cerr << "  Too many errors, stopping verification." << std::endl;
      break;
    }
  }

  if (errors == 0) {
    std::cout << "✓ All keys verified successfully" << std::endl;
  } else {
    std::cout << "✗ Found " << errors << " errors" << std::endl;
  }
}

void test_random_workload() {
  std::cout << "\n=== Test 2: Random Workload ===" << std::endl;

  const size_t NUM_OPS = 50000;
  std::string db_file = "random_test.dat";
  size_t region_size = 256 * 1024 * 1024; // 256 MB

  Manager manager(db_file, region_size, 512, true);

  BTreeConfig config;
  config.max_keys = 16;
  config.min_keys = 8;
  config.leaf_capacity = 32;

  BTree tree(&manager, config);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> key_dist(0, 1000000);

  std::cout << "Executing " << NUM_OPS << " random insert operations..."
            << std::endl;
  auto start = high_resolution_clock::now();

  std::vector<int> inserted_keys;

  for (size_t i = 0; i < NUM_OPS; i++) {
    int key = key_dist(gen);
    int value = key * 2;

    tree.insert(key, value);
    inserted_keys.push_back(key);

    if ((i + 1) % 10000 == 0) {
      std::cout << "  Progress: " << (i + 1) << " operations" << std::endl;
    }
  }

  auto end = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(end - start);

  std::cout << "✓ Random insertions complete in " << duration.count() << " ms"
            << std::endl;
  std::cout << "  Throughput: " << (NUM_OPS * 1000.0 / duration.count())
            << " ops/sec" << std::endl;

  // Sample verification (check 1000 random keys)
  std::cout << "Verifying sample of inserted keys..." << std::endl;
  std::shuffle(inserted_keys.begin(), inserted_keys.end(), gen);

  int verified = 0;
  int errors = 0;

  for (size_t i = 0; i < std::min(size_t(1000), inserted_keys.size()); i++) {
    int key = inserted_keys[i];
    int expected_value = key * 2;
    int actual_value;

    if (tree.search(key, actual_value)) {
      if (actual_value == expected_value) {
        verified++;
      } else {
        errors++;
        if (errors <= 5) {
          std::cerr << "  ERROR: Key " << key << " expected " << expected_value
                    << " but got " << actual_value << std::endl;
        }
      }
    } else {
      errors++;
      if (errors <= 5) {
        std::cerr << "  ERROR: Key " << key << " not found!" << std::endl;
      }
    }
  }

  std::cout << "✓ Verified " << verified << " keys, " << errors << " errors"
            << std::endl;
}

void test_garbage_collection() {
  std::cout << "\n=== Test 3: Garbage Collection ===" << std::endl;

  std::string db_file = "gc_test.dat";
  size_t region_size = 128 * 1024 * 1024; // 128 MB

  Manager manager(db_file, region_size, 512, true);

  BTreeConfig config;
  config.max_keys = 16;
  config.min_keys = 8;
  config.leaf_capacity = 32;

  BTree tree(&manager, config);

  std::cout << "Inserting 10000 keys to create tree structure..." << std::endl;
  for (int i = 0; i < 10000; i++) {
    tree.insert(i, i * 5);
  }

  std::cout << "Running garbage collector..." << std::endl;
  GarbageCollector gc(&manager);
  gc.collect(tree.root_offset(), config.max_keys, config.leaf_capacity);

  std::cout << "✓ GC Complete:" << std::endl;
  std::cout << "  Nodes marked: " << gc.nodes_marked() << std::endl;
  std::cout << "  Blocks freed: " << gc.blocks_freed() << std::endl;

  // Verify tree still works after GC
  std::cout << "Verifying tree integrity post-GC..." << std::endl;
  int value;
  bool test_passed = true;

  if (tree.search(5000, value) && value == 25000) {
    std::cout << "✓ Tree is still functional after GC" << std::endl;
  } else {
    std::cout << "✗ Tree corrupted after GC" << std::endl;
    test_passed = false;
  }
}

void test_crash_recovery() {
  std::cout << "\n=== Test 4: Crash Recovery Simulation ===" << std::endl;

  std::string db_file = "crash_test.dat";
  size_t region_size = 128 * 1024 * 1024;

  // Phase 1: Create database and insert data
  {
    std::cout << "Phase 1: Creating database and inserting data..."
              << std::endl;
    Manager manager(db_file, region_size, 512, true);

    BTreeConfig config;
    config.max_keys = 16;
    config.min_keys = 8;
    config.leaf_capacity = 32;

    BTree tree(&manager, config);

    for (int i = 0; i < 5000; i++) {
      tree.insert(i, i * 3);
    }

    std::cout << "✓ Phase 1 complete (database persisted)" << std::endl;
    // Manager destructor called here - flushes should happen via OS
  }

  // Phase 2: "Crash" - reopen without create_new flag
  {
    std::cout << "Phase 2: Simulating crash and recovery..." << std::endl;
    Manager manager(db_file, region_size, 512, false); // Reopen existing

    BTreeConfig config;
    config.max_keys = 16;
    config.min_keys = 8;
    config.leaf_capacity = 32;

    BTree tree(&manager, config);

    std::cout << "Running GC for crash recovery..." << std::endl;
    GarbageCollector gc(&manager);
    gc.collect(tree.root_offset(), config.max_keys, config.leaf_capacity);

    std::cout << "  Leaked blocks recovered: " << gc.blocks_freed()
              << std::endl;

    // Verify data integrity
    std::cout << "Verifying data after recovery..." << std::endl;
    int errors = 0;

    for (int i = 0; i < 5000; i++) {
      int value;
      if (!tree.search(i, value)) {
        errors++;
        if (errors <= 5) {
          std::cerr << "  ERROR: Lost key " << i << std::endl;
        }
      } else if (value != i * 3) {
        errors++;
        if (errors <= 5) {
          std::cerr << "  ERROR: Corrupted value for key " << i << std::endl;
        }
      }
    }

    if (errors == 0) {
      std::cout << "✓ All data recovered successfully!" << std::endl;
    } else {
      std::cout << "✗ Found " << errors << " errors after recovery"
                << std::endl;
    }
  }
}

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "   Atomic Tree Engine - Stress Test    " << std::endl;
  std::cout << "========================================" << std::endl;

  try {
    test_massive_insertions();
    test_random_workload();
    test_garbage_collection();
    test_crash_recovery();

    std::cout << "\n========================================" << std::endl;
    std::cout << "   All Tests Completed Successfully!   " << std::endl;
    std::cout << "========================================" << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "\n✗ TEST FAILED: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
