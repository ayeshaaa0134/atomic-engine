#include "manager.h"
#include "primitives.h"

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <stdexcept>
#include <format>
#include <bit>
#include <algorithm>
#include <iostream>

#ifdef _WIN32
#    include <windows.h>
#    include <psapi.h>
#else
#    include <sys/mman.h>
#    include <sys/stat.h>
#    include <fcntl.h>
#    include <unistd.h>
#endif

namespace atomic_tree {

consteval std::uint64_t magic_number() noexcept {
    return 0x4154524545;
}

constexpr std::size_t align_to_8(std::size_t value) noexcept {
    return (value + 7) & ~std::size_t{7};
}

constexpr std::size_t calculate_bitmap_words(std::size_t block_count) noexcept {
    return (block_count + 63) / 64;
}

Manager::Manager(const std::string &filename,
                 std::size_t region_size,
                 std::size_t block_size,
                 bool create_new)
    : region_size_(region_size),
      block_size_(block_size),
      file_handle_(0),
      map_handle_(0),
      base_(nullptr),
      metadata_(nullptr),
      bitmap_(nullptr),
      block_count_(0),
      bitmap_size_words_(0),
      allocated_blocks_(0) {
    block_count_ = region_size / block_size;
    bitmap_size_words_ = calculate_bitmap_words(block_count_);

#ifdef _WIN32
    DWORD access_mode = GENERIC_READ | GENERIC_WRITE;
    DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    DWORD creation_disp = create_new ? CREATE_ALWAYS : OPEN_ALWAYS;

    file_handle_ = CreateFileA(
        filename.c_str(),
        access_mode,
        share_mode,
        nullptr,
        creation_disp,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (file_handle_ == INVALID_HANDLE_VALUE) [[unlikely]] {
        throw std::runtime_error(
            std::format("Failed to create/open file: {} (Error: {})",
                        filename, GetLastError()));
    }

    LARGE_INTEGER size_li;
    size_li.QuadPart = static_cast<LONGLONG>(region_size);

    map_handle_ = CreateFileMappingA(
        file_handle_,
        nullptr,
        PAGE_READWRITE,
        size_li.HighPart,
        size_li.LowPart,
        nullptr
    );
    if (map_handle_ == nullptr) [[unlikely]] {
        DWORD err = GetLastError();
        CloseHandle(file_handle_);
        throw std::runtime_error(
            std::format("Failed to map file (Error: {})", err));
    }

    base_ = MapViewOfFile(
        map_handle_,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        region_size
    );
    if (base_ == nullptr) [[unlikely]] {
        CloseHandle(map_handle_);
        CloseHandle(file_handle_);
        throw std::runtime_error("Failed to view map");
    }
#else
    int flags = create_new ? (O_CREAT | O_RDWR) : O_RDWR;
    int fd = ::open(filename.c_str(), flags, 0644);
    if (fd < 0) [[unlikely]] {
        throw std::runtime_error(
            std::format("Failed to open file: {}", filename));
    }

    if (create_new) {
        if (::ftruncate(fd, static_cast<off_t>(region_size)) != 0) [[unlikely]] {
            ::close(fd);
            throw std::runtime_error("Failed to resize file");
        }
    }

    void *mapped = ::mmap(
        nullptr,
        region_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );
    if (mapped == MAP_FAILED) [[unlikely]] {
        ::close(fd);
        throw std::runtime_error("Failed to mmap file");
    }

    file_handle_ = fd;
    base_ = mapped;
#endif

    metadata_ = static_cast<Metadata *>(base_);
    std::size_t metadata_size = sizeof(Metadata);
    std::size_t bitmap_offset = align_to_8(metadata_size);

    bitmap_ = reinterpret_cast<std::uint64_t *>(
        static_cast<std::uint8_t *>(base_) + bitmap_offset);

    std::size_t bitmap_bytes = bitmap_size_words_ * sizeof(std::uint64_t);
    std::size_t total_reserved_bytes = bitmap_offset + bitmap_bytes;
    std::size_t reserved_blocks =
        (total_reserved_bytes + block_size - 1) / block_size;

    if (create_new) [[unlikely]] {
        metadata_->magic = magic_number();
        metadata_->version = 1;
        metadata_->root_offset = 0;
        metadata_->block_count = block_count_;
        metadata_->block_size = block_size_;
        metadata_->max_keys = 16;
        metadata_->min_keys = 8;
        metadata_->leaf_capacity = 32;
        metadata_->padding = 0;

        std::memset(bitmap_, 0, bitmap_bytes);

        for (std::size_t i = 0; i < reserved_blocks; ++i) {
            std::size_t word_idx = i / 64;
            std::size_t bit_idx = i % 64;
            bitmap_[word_idx] |= (1ULL << bit_idx);
        }

        allocated_blocks_ = reserved_blocks;

        update_persistent_checksum();
        persist(metadata_, sizeof(Metadata));
        persist(bitmap_, bitmap_bytes);
    } else [[likely]] {
        if (metadata_->magic != magic_number()) [[unlikely]] {
            // Magic mismatch; leave handling to caller or future logic.
        }

        allocated_blocks_ = 0;
        for (std::size_t i = 0; i < bitmap_size_words_; ++i) {
            allocated_blocks_ += static_cast<std::size_t>(
                std::popcount(bitmap_[i]));
        }

        if (!create_new) [[likely]] {
            if (!verify_integrity()) [[unlikely]] {
                std::cerr
                    << R"({"type": "log", "level": "ERROR", "message": "NVM Integrity Failure"})"
                    << std::endl;
            }
        }
    }
}

void Manager::set_root_offset(std::uint64_t offset) {
    metadata_->root_offset = offset;
    update_persistent_checksum();
    _mm_clflush(metadata_);
    _mm_sfence();
}

[[nodiscard]] std::uint64_t Manager::get_root_offset() const noexcept {
    return metadata_->root_offset;
}

Manager::~Manager() {
#ifdef _WIN32
    if (base_) {
        UnmapViewOfFile(base_);
    }
    if (map_handle_) {
        CloseHandle(map_handle_);
    }
    if (file_handle_ && file_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(file_handle_);
    }
#else
    if (base_) {
        ::munmap(base_, region_size_);
    }
    if (file_handle_ >= 0) {
        ::close(file_handle_);
    }
#endif
}

[[nodiscard]] std::uint64_t Manager::alloc_block() {
    for (std::size_t i = 0; i < bitmap_size_words_; ++i) {
        std::uint64_t word = bitmap_[i];
        if (word != ~0ULL) [[likely]] {
            std::uint64_t inverted = ~word;
            auto index = std::countr_zero(inverted);
            bitmap_[i] |= (1ULL << index);

            std::size_t block_idx = i * 64 + index;
            if (block_idx >= block_count_) [[unlikely]] {
                throw std::runtime_error("Out of memory (bitmap edge)");
            }

            allocated_blocks_++;
            return static_cast<std::uint64_t>(block_idx * block_size_);
        }
    }

    throw std::runtime_error("Out of memory");
}

void Manager::free_block(std::uint64_t offset) {
    if (offset >= region_size_) [[unlikely]]
        return;

    std::size_t block_idx = static_cast<std::size_t>(offset / block_size_);
    std::size_t word_idx = block_idx / 64;
    std::size_t bit_idx = block_idx % 64;

    if (bitmap_[word_idx] & (1ULL << bit_idx)) [[likely]] {
        bitmap_[word_idx] &= ~(1ULL << bit_idx);
        if (allocated_blocks_ > 0) {
            allocated_blocks_--;
        }
    }
}

[[nodiscard]] void *Manager::offset_to_ptr(std::uint64_t offset) noexcept {
    return static_cast<std::uint8_t *>(base_) + offset;
}

[[nodiscard]] void *Manager::base() const noexcept {
    return base_;
}

[[nodiscard]] std::size_t Manager::region_size() const noexcept {
    return region_size_;
}

[[nodiscard]] std::size_t Manager::block_size() const noexcept {
    return block_size_;
}

[[nodiscard]] std::size_t Manager::block_count() const noexcept {
    return block_count_;
}

[[nodiscard]] std::uint64_t *Manager::get_bitmap() noexcept {
    return bitmap_;
}

void Manager::print_telemetry(double ops_per_sec, double latency_us) {
    std::uint64_t rss = get_real_rss();
    std::uint64_t logical_writes =
        static_cast<std::uint64_t>(ops_per_sec * 16.0);

    std::cout << std::format(
                     R"({{"type": "metric", "ops": {}, "latency": {}, "mem_used": {}, "physical_writes": {}, "logical_writes": {}, "allocated_blocks": {}, "treeType": "B+ Tree", "consistency": "Shadow Paging", "version": "1.1.0", "integrity": "{}", "region_kb": {}, "block_size": {}}})",
                     ops_per_sec,
                     latency_us,
                     rss,
                     total_persisted_bytes,
                     logical_writes,
                     allocated_blocks_,
                     (verify_integrity() ? "PASSED" : "FAILED"),
                     (region_size_ / 1024),
                     block_size_)
              << std::endl;

    std::string hex_data;
    for (std::size_t i = 0;
         i < std::min(bitmap_size_words_, std::size_t{16});
         ++i) {
        hex_data += std::format("{:016x}", bitmap_[i]);
    }

    std::cout << std::format(
                     R"({{"type": "bitmap", "data": "{}", "offset": 0}})",
                     hex_data)
              << std::endl;
}

[[nodiscard]] std::uint64_t Manager::get_real_rss() noexcept {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&pmc),
            sizeof(pmc))) [[likely]] {
        return static_cast<std::uint64_t>(pmc.WorkingSetSize);
    }
    return 0;
#else
    // Portable RSS retrieval could be added here; return 0 as in original.
    return 0;
#endif
}

[[nodiscard]] std::uint64_t Manager::calculate_checksum() const noexcept {
    std::uint64_t checksum = 0;

    const std::uint64_t *ptr =
        static_cast<const std::uint64_t *>(base_);
    std::size_t words = region_size_ / sizeof(std::uint64_t);

    for (std::size_t i = 0; i < words; ++i) {
        if (ptr + i == &metadata_->checksum) [[unlikely]]
            continue;

        std::uint64_t val = ptr[i];
        checksum ^= std::rotl(val, 1);
    }

    return checksum;
}

[[nodiscard]] std::uint32_t calculate_node_checksum(const void *data,
                                                    std::size_t len) noexcept {
    const std::uint8_t *byte_ptr =
        static_cast<const std::uint8_t *>(data);
    std::uint32_t crc = 0xFFFFFFFF;

    for (std::size_t i = 0; i < len; ++i) {
        if (i >= 5 && i < 9) [[unlikely]]
            continue;

        crc ^= byte_ptr[i];
        for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }

    return ~crc;
}

void Manager::update_persistent_checksum() {
    metadata_->checksum = calculate_checksum();
    _mm_clflush(&metadata_->checksum);
    _mm_sfence();
}

[[nodiscard]] bool Manager::verify_integrity() const noexcept {
    if (!base_ || metadata_->magic != magic_number()) [[unlikely]]
        return false;

    std::uint64_t current = calculate_checksum();
    return (current == metadata_->checksum);
}

} // namespace atomic_tree
