#include "manager.h"
#include <cstring>
#include <immintrin.h> // For _tzcnt_u64
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
// Fallback for Linux/POSIX if needed (user is on Windows though)
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace atomic_tree {

Manager::Manager(const std::string &filename, std::size_t region_size,
                 std::size_t block_size, bool create_new)
    : region_size_(region_size), block_size_(block_size) {

  block_count_ = region_size / block_size;
  bitmap_size_words_ = (block_count_ + 63) / 64;

#ifdef _WIN32
  // Windows Memory Mapping
  DWORD access_mode = GENERIC_READ | GENERIC_WRITE;
  DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
  DWORD creation_disp = create_new ? CREATE_ALWAYS : OPEN_ALWAYS;

  file_handle_ = CreateFileA(filename.c_str(), access_mode, share_mode, nullptr,
                             creation_disp, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file_handle_ == INVALID_HANDLE_VALUE) {
    throw std::runtime_error("Failed to create/open file: " + filename);
  }

  // Set file size if creating/expanding
  LARGE_INTEGER size_li;
  size_li.QuadPart = region_size;
  // We strictly map the requested size.
  // If opening existing, we assume it's large enough or we expand it?
  // Let's set the size to ensure it works.
  // But if it's OPEN_ALWAYS and exists, setting size might be optional but
  // good. Wait, CreateFileMapping with size will expand it.

  map_handle_ = CreateFileMappingA(file_handle_, nullptr, PAGE_READWRITE,
                                   size_li.HighPart, size_li.LowPart, nullptr);
  if (map_handle_ == nullptr) {
    CloseHandle(file_handle_);
    throw std::runtime_error("Failed to map file");
  }

  base_ = MapViewOfFile(map_handle_, FILE_MAP_ALL_ACCESS, 0, 0, region_size);
  if (base_ == nullptr) {
    CloseHandle(map_handle_);
    CloseHandle(file_handle_);
    throw std::runtime_error("Failed to view map");
  }
#endif

  // Setup Bitmap
  bitmap_ = static_cast<std::uint64_t *>(base_);

  // Mark reserved blocks for bitmap itself
  std::size_t bitmap_bytes = bitmap_size_words_ * 8;
  std::size_t reserved_blocks = (bitmap_bytes + block_size - 1) / block_size;

  if (create_new) {
    // Zero out bitmap
    std::memset(bitmap_, 0, bitmap_bytes);

    // Mark reserved
    for (std::size_t i = 0; i < reserved_blocks; ++i) {
      std::size_t word_idx = i / 64;
      std::size_t bit_idx = i % 64;
      bitmap_[word_idx] |= (1ULL << bit_idx);
    }
  }
}

Manager::~Manager() {
#ifdef _WIN32
  if (base_)
    UnmapViewOfFile(base_);
  if (map_handle_)
    CloseHandle(map_handle_);
  if (file_handle_ && file_handle_ != INVALID_HANDLE_VALUE)
    CloseHandle(file_handle_);
#endif
}

std::uint64_t Manager::alloc_block() {
  // Scan bitmap for free block
  // Portable implementation - works on all CPUs

  for (std::size_t i = 0; i < bitmap_size_words_; ++i) {
    std::uint64_t word = bitmap_[i];
    if (word != ~0ULL) { // Found a word with free space
      // Find first zero bit (manual implementation, portable)
      std::uint64_t inverted = ~word;

      // Count trailing zeros (portable way)
      unsigned long long index = 0;
      if (inverted != 0) {
        // Find first set bit in inverted (= first zero in original)
        while ((inverted & 1) == 0) {
          inverted >>= 1;
          index++;
        }
      }

      // Set the bit
      bitmap_[i] |= (1ULL << index);

      // Calculate absolute block index
      std::size_t block_idx = i * 64 + index;

      if (block_idx >= block_count_) {
        throw std::runtime_error("Out of memory (bitmap edge)");
      }

      // Tracking real allocation
      allocated_blocks_++;

      // Return offset
      return block_idx * block_size_;
    }
  }

  throw std::runtime_error("Out of memory");
}

void Manager::free_block(std::uint64_t offset) {
  if (offset >= region_size_)
    return;

  std::size_t block_idx = offset / block_size_;
  std::size_t word_idx = block_idx / 64;
  std::size_t bit_idx = block_idx % 64;

  // Clear bit with real tracking
  if (bitmap_[word_idx] & (1ULL << bit_idx)) {
    bitmap_[word_idx] &= ~(1ULL << bit_idx);
    if (allocated_blocks_ > 0)
      allocated_blocks_--;
  }
}

void *Manager::offset_to_ptr(std::uint64_t offset) {
  return static_cast<char *>(base_) + offset;
}

void *Manager::base() const { return base_; }
std::size_t Manager::region_size() const { return region_size_; }
std::size_t Manager::block_size() const { return block_size_; }
std::size_t Manager::block_count() const { return block_count_; }

std::uint64_t *Manager::get_bitmap() { return bitmap_; }

void Manager::print_telemetry(double ops_per_sec, double latency_us) {
  std::uint64_t rss = get_real_rss();

  // Get total system memory (Real actual system info)
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  GlobalMemoryStatusEx(&statex);

  // Output JSON to stdout - Extension will parse this
  std::cout << "{\"type\": \"metric\", \"ops\": " << ops_per_sec
            << ", \"latency\": " << latency_us << ", \"mem_used\": " << rss
            << ", \"total_sys_mem\": " << statex.ullTotalPhys
            << ", \"allocated_nodes\": " << (allocated_blocks_ * block_size_)
            << "}" << std::endl;

  // Send memory map snapshot (Partial for performance)
  std::cout << "{\"type\": \"memory\", \"blocks\": [";
  for (std::size_t i = 0; i < std::min(block_count_, (std::size_t)600); ++i) {
    std::size_t word_idx = i / 64;
    std::size_t bit_idx = i % 64;
    bool is_alloc = (bitmap_[word_idx] & (1ULL << bit_idx)) != 0;

    std::cout << "{\"id\":" << i << ", \"type\":\""
              << (is_alloc ? "allocated" : "free") << "\"}";
    if (i < std::min(block_count_, (std::size_t)600) - 1)
      std::cout << ",";
  }
  std::cout << "]}" << std::endl;
}

#include <psapi.h>
std::uint64_t Manager::get_real_rss() {
#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS_EX pmc;
  if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc,
                           sizeof(pmc))) {
    return pmc.WorkingSetSize;
  }
#endif
  return 0;
}

} // namespace atomic_tree
