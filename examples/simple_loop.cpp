#include "../backend/include/B_tree.h"
#include "../backend/include/manager.h"
#include <chrono>
#include <iostream>


using namespace atomic_tree;
using namespace std;

int main() {
  cout << "{\"type\": \"log\", \"level\": \"info\", \"message\": \"Starting "
          "AtomicTree Simple Example...\"}"
       << endl;

  // Create database (small 64MB for quick demo)
  Manager mgr("simple_demo.dat", 64 * 1024 * 1024, 256, true);

  BTreeConfig cfg{16, 8, 32};
  BTree tree(&mgr, cfg);

  auto start_all = chrono::high_resolution_clock::now();

  // Simple loop - insert 1000 numbers
  for (int i = 0; i < 1000; i++) {
    auto start_op = chrono::high_resolution_clock::now();
    tree.insert(i, i * 2);
    auto end_op = chrono::high_resolution_clock::now();

    // Show progress in UI
    if (i % 100 == 0) {
      double latency =
          chrono::duration<double, chrono::micro>(end_op - start_op).count();
      double throughput =
          (i + 1) * 1000000.0 /
          chrono::duration<double, chrono::micro>(end_op - start_all).count();
      mgr.print_telemetry(throughput, latency);
    }
  }

  // Search for a value
  int value;
  if (tree.search(500, value)) {
    cout << "{\"type\": \"log\", \"level\": \"event\", \"message\": \"Found "
            "key 500: "
         << value << "\"}" << endl;
  }

  cout << "{\"type\": \"log\", \"level\": \"info\", \"message\": \"Example "
          "Done!\"}"
       << endl;
  return 0;
}
