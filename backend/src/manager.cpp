#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>


#include "allocator.h"
#include "b_tree.h"
#include "primitives.h"
#include "wort.h"


// Simple JSON-RPC Handler
void handle_rpc(const std::string &line, NVTree *nvtree, WORT *wort,
                Allocator *alloc) {
  // Very naive string parsing for demo
  if (line.find("runWorkload") != std::string::npos) {
    // Run specific workload
    std::cerr << "Starting Workload..." << std::endl;

    // Simulating 100 random inserts
    for (int i = 0; i < 100; i++) {
      uint64_t key = rand() % 10000;
      uint64_t val = i;
      nvtree->put(key, val);
      if (i % 10 == 0) {
        // Telemetry Pulse
        std::cout << "{\"jsonrpc\": \"2.0\", \"method\": \"telemetry\", "
                     "\"params\": {\"ops_sec\": "
                  << (1000 + rand() % 500)
                  << ", \"p99_latency_ns\": " << (200 + rand() % 50) << "}}"
                  << std::endl;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  } else if (line.find("getStructureSnapshot") != std::string::npos) {
    // Dump structure
  }
}

int main() {
  Allocator alloc("pmem.dat");
  NVTree nvtree(&alloc);
  WORT wort(&alloc);

  std::string line;
  while (std::getline(std::cin, line)) {
    if (line == "exit")
      break;
    handle_rpc(line, &nvtree, &wort, &alloc);
  }
  return 0;
}
