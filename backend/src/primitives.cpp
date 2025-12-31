#include "primitives.h"
#include <chrono>
#include <mutex>

static std::vector<TraceEvent> trace_buffer;
static std::mutex trace_mutex;

void Primitives::output_fence() {
  _mm_sfence();
  record_trace(OpType::FENCE);
}

void Primitives::flush(void *addr) {
  _mm_clflush(addr); // Standard CLFLUSH for compatibility
  record_trace(OpType::FLUSH, (uint64_t)addr);
}

void Primitives::nontemporal_store(void *addr, uint64_t val) {
  _mm_stream_si64((long long *)addr, val);
  record_trace(OpType::STORE_BYPASS, (uint64_t)addr);
}

void Primitives::record_trace(OpType type, uint64_t addr) {
  std::lock_guard<std::mutex> lock(trace_mutex);
  uint64_t ts =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  trace_buffer.push_back({type, addr, ts});
}

std::vector<TraceEvent> Primitives::get_and_clear_traces() {
  std::lock_guard<std::mutex> lock(trace_mutex);
  std::vector<TraceEvent> copy = trace_buffer;
  trace_buffer.clear();
  return copy;
}
