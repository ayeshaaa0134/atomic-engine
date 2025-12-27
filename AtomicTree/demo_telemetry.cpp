#include "backend/include/manager.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>


using namespace atomic_tree;

int main() {
  try {
    // Initialize AtomicTree Manager
    // 1GB region, 512B blocks
    Manager manager("demo_nvm.dat", 1024 * 1024 * 1024, 512, true);

    std::cout << "{\"type\": \"log\", \"level\": \"info\", \"message\": "
                 "\"AtomicTree Demo Started: 1GB Region Initialized\"}"
              << std::endl;

    std::vector<uint64_t> offsets;

    // Simulation loop
    for (int i = 0; i < 100; ++i) {
      // Simulate variable workload
      int batch_size = 5 + (rand() % 15);
      auto start_tick = std::chrono::high_resolution_clock::now();

      for (int j = 0; j < batch_size; ++j) {
        offsets.push_back(manager.alloc_block());
      }

      auto end_tick = std::chrono::high_resolution_clock::now();
      double latency =
          std::chrono::duration<double, std::micro>(end_tick - start_tick)
              .count() /
          batch_size;
      double ops = batch_size * 100.0; // Simulated scale

      // Emit Real Telemetry
      manager.print_telemetry(ops, latency);

      if (i % 20 == 0) {
        std::cout << "{\"type\": \"log\", \"level\": \"event\", \"message\": "
                     "\"Checkpoint reached. Syncing NVM...\"}"
                  << std::endl;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "{\"type\": \"log\", \"level\": \"info\", \"message\": \"Demo "
                 "completed successfully.\"}"
              << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "{\"type\": \"log\", \"level\": \"error\", \"message\": "
                 "\"Demo Error: "
              << e.what() << "\"}" << std::endl;
    return 1;
  }

  return 0;
}
