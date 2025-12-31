#include "allocator.h"
#include "b_tree.h"
#include <iostream>


int main() {
  Allocator alloc("pmem.dat"); // Re-open existing
  NVTree tree(&alloc);

  tree.reconstruct(); // Rebuild from PM logs

  uint64_t val;
  if (tree.get(100, val)) {
    std::cout << "Recovered Key 100: " << val << std::endl;
  } else {
    std::cout << "Failed to find Key 100" << std::endl;
  }
  return 0;
}
