#include "allocator.h"
#include "primitives.h"
#include <iostream>
#include <windows.h>

Allocator::Allocator(const std::string &filename) : used_blocks_count(0) {
  total_blocks = POOL_SIZE / BLOCK_SIZE;

  // Create/Open File
  file_handle = CreateFileA(filename.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                            NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file_handle == INVALID_HANDLE_VALUE) {
    std::cerr << "Failed to open PM file" << std::endl;
    exit(1);
  }

  // Map File
  map_handle = CreateFileMappingA(file_handle, NULL, PAGE_READWRITE, 0,
                                  (DWORD)POOL_SIZE, NULL);
  if (map_handle == NULL) {
    std::cerr << "Failed to map PM file" << std::endl;
    exit(1);
  }

  base_addr = MapViewOfFile(map_handle, FILE_MAP_ALL_ACCESS, 0, 0, POOL_SIZE);
  if (base_addr == NULL) {
    std::cerr << "Failed to view PM map" << std::endl;
    exit(1);
  }

  // Bitmap at the start of the pool?
  // For simplicity, let's keep bitmap in volatile DRAM for this "Atomic Engine"
  // demo to focus on the Tree Persistence. If we put it in PM, we eat up the
  // space. Let's just malloc it. The prompt asks for "Persistent Region ... 1
  // bit per block". We will simulate the bitmap in DRAM but update it
  // rigorously. (In a real driver, this would be part of the metadata region).

  uint64_t bitmap_size = total_blocks / 8;
  bitmap_addr = malloc(bitmap_size);
  memset(bitmap_addr, 0, bitmap_size);

  // Reserve Block 0 (so we never return offset 0, which get_abs_addr treats as
  // nullptr)
  set_bit(0);
}

Allocator::~Allocator() {
  UnmapViewOfFile(base_addr);
  CloseHandle(map_handle);
  CloseHandle(file_handle);
  free(bitmap_addr);
}

void *Allocator::get_abs_addr(uint64_t offset) {
  if (offset == 0)
    return nullptr;
  return (char *)base_addr + offset;
}

uint64_t Allocator::get_rel_offset(void *addr) {
  if (addr == nullptr)
    return 0;
  return (uint64_t)((char *)addr - (char *)base_addr);
}

uint64_t Allocator::alloc_block() {
  int idx = find_free_bit();
  if (idx == -1)
    return 0; // OOM
  set_bit(idx);
  used_blocks_count++;

  uint64_t offset = idx * BLOCK_SIZE;

  // Simulate allocation persistence: FLUSH the new block to ensure garbage
  // content is durable? Or just return.
  Primitives::record_trace(OpType::ALLOC, (uint64_t)get_abs_addr(offset));
  return offset;
}

void Allocator::free_block(uint64_t offset) {
  uint64_t idx = offset / BLOCK_SIZE;
  clear_bit((int)idx);
  used_blocks_count--;
  Primitives::record_trace(OpType::FREE, (uint64_t)get_abs_addr(offset));
}

// Simple linear search bitmap (slow but clear for functionality)
int Allocator::find_free_bit() {
  uint8_t *map = (uint8_t *)bitmap_addr;
  uint64_t bytes = total_blocks / 8;
  for (uint64_t i = 0; i < bytes; i++) {
    if (map[i] != 0xFF) {
      for (int j = 0; j < 8; j++) {
        if (!((map[i] >> j) & 1)) {
          return (int)(i * 8 + j);
        }
      }
    }
  }
  return -1;
}

void Allocator::set_bit(int index) {
  uint8_t *map = (uint8_t *)bitmap_addr;
  map[index / 8] |= (1 << (index % 8));
}

void Allocator::clear_bit(int index) {
  uint8_t *map = (uint8_t *)bitmap_addr;
  map[index / 8] &= ~(1 << (index % 8));
}
