#pragma once
#include <cstdint>
#include <atomic>
#include <vector>
#include <string>
#include <immintrin.h>

// Operation Types for Telemetry
enum class OpType {
    FLUSH,
    FENCE,
    STORE_BYPASS, // Non-temporal store
    ATOMIC_STORE,
    ALLOC,
    FREE
};

struct TraceEvent {
    OpType type;
    uint64_t address;
    uint64_t timestamp;
};

class Primitives {
public:
    // Memory Fence (SFENCE)
    static void input_fence(); // LFENCE
    static void output_fence(); // SFENCE
    static void full_fence(); // MFENCE

    // Cache line flush
    static void flush(void* addr); // CLFLUSHOPT or CLWB
    
    // Non-temporal store (bypass cache)
    static void nontemporal_store(void* addr, uint64_t val);

    // Telemetry / Radar
    static void record_trace(OpType type, uint64_t addr = 0);
    static std::vector<TraceEvent> get_and_clear_traces();
};

// 8-byte failure-atomic aligned pointer wrapper
struct AtomicPtr {
    alignas(8) std::atomic<uint64_t> offset;

    void store(uint64_t val) {
        // x86-64 release store is atomic for 8-byte aligned
        offset.store(val, std::memory_order_release);
        Primitives::record_trace(OpType::ATOMIC_STORE, (uint64_t)&offset);
    }

    uint64_t load() const {
        return offset.load(std::memory_order_acquire);
    }
};
