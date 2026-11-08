// Benchmarks for nbytes, which implements the legacy base64/hex codecs,
// byte swapping and string search used by `Buffer`.

#include <benchmark/benchmark.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

// nbytes' string search expects the embedder (Node.js' src/util.h) to
// provide CHECK().
#ifndef CHECK
#define CHECK(expr) assert(expr)
#endif

#include "nbytes.h"

static std::vector<char> RandomBytes(size_t size) {
  std::vector<char> data(size);
  uint32_t seed = 0x12345678;
  for (auto& c : data) {
    seed = seed * 1664525u + 1013904223u;
    c = static_cast<char>(seed >> 24);
  }
  return data;
}

static std::string Base64Encode(const std::vector<char>& data) {
  static const char kTable[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  size_t i = 0;
  for (; i + 2 < data.size(); i += 3) {
    uint32_t v = (static_cast<uint8_t>(data[i]) << 16) |
                 (static_cast<uint8_t>(data[i + 1]) << 8) |
                 static_cast<uint8_t>(data[i + 2]);
    out += kTable[(v >> 18) & 63];
    out += kTable[(v >> 12) & 63];
    out += kTable[(v >> 6) & 63];
    out += kTable[v & 63];
  }
  if (i + 1 == data.size()) {
    uint32_t v = static_cast<uint8_t>(data[i]) << 16;
    out += kTable[(v >> 18) & 63];
    out += kTable[(v >> 12) & 63];
    out += "==";
  } else if (i + 2 == data.size()) {
    uint32_t v = (static_cast<uint8_t>(data[i]) << 16) |
                 (static_cast<uint8_t>(data[i + 1]) << 8);
    out += kTable[(v >> 18) & 63];
    out += kTable[(v >> 12) & 63];
    out += kTable[(v >> 6) & 63];
    out += '=';
  }
  return out;
}

static void BM_Base64Decode(benchmark::State& state) {
  const std::string encoded =
      Base64Encode(RandomBytes(static_cast<size_t>(state.range(0))));
  std::vector<char> out(nbytes::Base64DecodedSize(encoded.data(),
                                                  encoded.size()));
  for (auto _ : state) {
    size_t written = nbytes::Base64Decode(out.data(), out.size(),
                                          encoded.data(), encoded.size());
    benchmark::DoNotOptimize(written);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * encoded.size());
}
BENCHMARK(BM_Base64Decode)->Arg(64)->Arg(64 * 1024);

static void BM_Base64DecodeWithWhitespace(benchmark::State& state) {
  // Line-wrapped input (e.g. PEM) forces the slow path.
  const std::string raw = Base64Encode(RandomBytes(16 * 1024));
  std::string encoded;
  for (size_t i = 0; i < raw.size(); i += 64) {
    encoded += raw.substr(i, 64);
    encoded += '\n';
  }
  std::vector<char> out(raw.size());
  for (auto _ : state) {
    size_t written = nbytes::Base64Decode(out.data(), out.size(),
                                          encoded.data(), encoded.size());
    benchmark::DoNotOptimize(written);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * encoded.size());
}
BENCHMARK(BM_Base64DecodeWithWhitespace);

static void BM_HexEncode(benchmark::State& state) {
  const auto data = RandomBytes(static_cast<size_t>(state.range(0)));
  std::vector<char> out(data.size() * 2);
  for (auto _ : state) {
    size_t written =
        nbytes::HexEncode(data.data(), data.size(), out.data(), out.size());
    benchmark::DoNotOptimize(written);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_HexEncode)->Arg(64)->Arg(64 * 1024);

static void BM_HexDecode(benchmark::State& state) {
  const auto data = RandomBytes(static_cast<size_t>(state.range(0)));
  const std::string hex = nbytes::HexEncode(data.data(), data.size());
  std::vector<char> out(data.size());
  for (auto _ : state) {
    size_t written =
        nbytes::HexDecode(out.data(), out.size(), hex.data(), hex.size());
    benchmark::DoNotOptimize(written);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * hex.size());
}
BENCHMARK(BM_HexDecode)->Arg(64)->Arg(64 * 1024);

static void BM_SwapBytes(benchmark::State& state) {
  auto data = RandomBytes(64 * 1024);
  const int width = static_cast<int>(state.range(0));
  for (auto _ : state) {
    bool ok = false;
    switch (width) {
      case 16:
        ok = nbytes::SwapBytes16(data.data(), data.size());
        break;
      case 32:
        ok = nbytes::SwapBytes32(data.data(), data.size());
        break;
      case 64:
        ok = nbytes::SwapBytes64(data.data(), data.size());
        break;
    }
    benchmark::DoNotOptimize(ok);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_SwapBytes)->Arg(16)->Arg(32)->Arg(64);

static void BM_SearchString(benchmark::State& state) {
  // Buffer#indexOf over a large haystack where the needle sits at the end.
  std::string haystack;
  while (haystack.size() < 256 * 1024) {
    haystack += "The quick brown fox jumps over the lazy dog. ";
  }
  const std::string needle = "needle-in-a-haystack";
  haystack += needle;
  const bool forward = state.range(0) != 0;
  for (auto _ : state) {
    size_t pos = nbytes::SearchString(
        reinterpret_cast<const uint8_t*>(haystack.data()), haystack.size(),
        reinterpret_cast<const uint8_t*>(needle.data()), needle.size(),
        forward ? 0 : haystack.size(), forward);
    benchmark::DoNotOptimize(pos);
  }
  state.SetBytesProcessed(state.iterations() * haystack.size());
}
BENCHMARK(BM_SearchString)->ArgName("forward")->Arg(1)->Arg(0);

BENCHMARK_MAIN();
