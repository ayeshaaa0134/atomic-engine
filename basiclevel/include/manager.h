#ifndef ATOMIC_TREE_MANAGER_H
#define ATOMIC_TREE_MANAGER_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace atomic_tree {

class Manager {
public:
    struct Metadata {
        std::uint64_t magic;
        std::uint32_t version;
        std::uint32_t padding;
        std::uint64_t root_offset;
        std::uint64_t block_count;
        std::uint64_t block_size;
        std::uint32_t max_keys;
        std::uint32_t min_keys;
        std::uint32_t leaf_capacity;
        std::uint32_t reserved;   // keep alignment
        std::uint64_t checksum;
    };

    Manager(const std::string &filename,
            std::size_t region_size,
            std::size_t block_size,
            bool create_new);

    ~Manager();

    void set_root_offset(std::uint64_t offset);
    [[nodiscard]] std::uint64_t get_root_offset() const noexcept;

    [[nodiscard]] std::uint64_t alloc_block();
    void free_block(std::uint64_t offset);

    [[nodiscard]] void *offset_to_ptr(std::uint64_t offset) noexcept;
    [[nodiscard]] void *base() const noexcept;

    [[nodiscard]] std::size_t region_size() const noexcept;
    [[nodiscard]] std::size_t block_size() const noexcept;
    [[nodiscard]] std::size_t block_count() const noexcept;

    [[nodiscard]] std::uint64_t *get_bitmap() noexcept;

    void print_telemetry(double ops_per_sec, double latency_us);

    [[nodiscard]] std::uint64_t get_real_rss() noexcept;

    [[nodiscard]] std::uint64_t calculate_checksum() const noexcept;
    void update_persistent_checksum();
    [[nodiscard]] bool verify_integrity() const noexcept;

private:
    std::size_t region_size_;
    std::size_t block_size_;

#ifdef _WIN32
    void *file_handle_;
    void *map_handle_;
#else
    int   file_handle_;
    void *map_handle_;   // unused on POSIX, but keeps layout same in ctor
#endif

    void          *base_;
    Metadata      *metadata_;
    std::uint64_t *bitmap_;
    std::size_t    block_count_;
    std::size_t    bitmap_size_words_;
    std::size_t    allocated_blocks_;
};

[[nodiscard]] std::uint32_t calculate_node_checksum(const void *data,
                                                    std::size_t len) noexcept;

} // namespace atomic_tree

#endif // ATOMIC_TREE_MANAGER_H
