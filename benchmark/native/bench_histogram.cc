// Benchmarks for hdr_histogram, which backs `perf_hooks.createHistogram()`
// and `monitorEventLoopDelay()`.

#include <benchmark/benchmark.h>

#include <cstdint>
#include <vector>

extern "C" {
#include "hdr/hdr_histogram.h"
}

// Same configuration as node::Histogram defaults: 1ns .. INT64_MAX, 3 digits.
static struct hdr_histogram* NewHistogram() {
  struct hdr_histogram* h = nullptr;
  hdr_init(1, INT64_MAX, 3, &h);
  return h;
}

static std::vector<int64_t> Samples(size_t n) {
  std::vector<int64_t> values(n);
  uint64_t seed = 0x9E3779B97F4A7C15ull;
  for (auto& v : values) {
    seed ^= seed << 13;
    seed ^= seed >> 7;
    seed ^= seed << 17;
    // Latencies between ~1us and ~100ms.
    v = 1000 + static_cast<int64_t>(seed % 100000000);
  }
  return values;
}

static void BM_HistogramRecord(benchmark::State& state) {
  struct hdr_histogram* h = NewHistogram();
  const auto values = Samples(4096);
  for (auto _ : state) {
    for (int64_t v : values) {
      benchmark::DoNotOptimize(hdr_record_value(h, v));
    }
  }
  state.SetItemsProcessed(state.iterations() * values.size());
  hdr_close(h);
}
BENCHMARK(BM_HistogramRecord);

static void BM_HistogramPercentiles(benchmark::State& state) {
  struct hdr_histogram* h = NewHistogram();
  for (int64_t v : Samples(100000)) hdr_record_value(h, v);
  const double percentiles[] = {50.0, 75.0, 90.0, 99.0, 99.9, 99.99};
  for (auto _ : state) {
    for (double p : percentiles) {
      benchmark::DoNotOptimize(hdr_value_at_percentile(h, p));
    }
    benchmark::DoNotOptimize(hdr_mean(h));
    benchmark::DoNotOptimize(hdr_stddev(h));
    benchmark::DoNotOptimize(hdr_min(h));
    benchmark::DoNotOptimize(hdr_max(h));
  }
  hdr_close(h);
}
BENCHMARK(BM_HistogramPercentiles);

static void BM_HistogramAdd(benchmark::State& state) {
  struct hdr_histogram* src = NewHistogram();
  for (int64_t v : Samples(10000)) hdr_record_value(src, v);
  struct hdr_histogram* dst = NewHistogram();
  for (auto _ : state) {
    benchmark::DoNotOptimize(hdr_add(dst, src));
  }
  hdr_close(src);
  hdr_close(dst);
}
BENCHMARK(BM_HistogramAdd);

BENCHMARK_MAIN();
