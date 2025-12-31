#include "allocator.h"
#include "b_tree.h"
#include "wort.h"
#include <iostream>
#include <thread>
#include <vector>


int main() {
  Allocator alloc("pmem_stress.dat");
  NVTree tree(&alloc);

  std::vector<std::thread> threads;

  // Simple Stress: Insert 10k items
  for (int i = 0; i < 10000; i++) {
    tree.put(i, i * 2);
    if (i % 1000 == 0)
      std::cout << "Inserted " << i << std::endl;
  }

  std::cout << "Stress Test Complete" << std::endl;
  return 0;
}
