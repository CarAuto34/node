// Benchmarks for ada, the WHATWG URL parser backing `new URL()`,
// `URL.canParse()` and `url.parse()` in Node.js.

#include <benchmark/benchmark.h>

#include <string>
#include <string_view>
#include <vector>

#include "ada.h"

static const std::vector<std::string_view>& RealisticUrls() {
  static const std::vector<std::string_view> urls = {
      "https://nodejs.org/en/download",
      "https://www.google.com/search?q=node.js&oq=node&sourceid=chrome",
      "http://user:pass@example.com:8080/path/to/resource?query=1#fragment",
      "https://registry.npmjs.org/@types%2fnode/-/node-20.11.0.tgz",
      "file:///home/user/projects/app/node_modules/lodash/index.js",
      "https://github.com/nodejs/node/blob/main/lib/internal/url.js#L100",
      "http://[2001:db8::1]:3000/api/v1/items?limit=100&offset=200",
      "ws://localhost:9229/1a2b3c4d-5e6f-7a8b-9c0d-1e2f3a4b5c6d",
      "https://xn--nxasmq6b.com/caf%C3%A9?lang=fr",
      "https://例え.テスト/パス?クエリ=値",
      "data:text/plain;base64,SGVsbG8sIFdvcmxkIQ==",
      "node:fs",
      "https://example.com/a/b/../c/./d/../../e?x=%20y",
      "HTTPS://EXAMPLE.COM:443/UPPER/Case/Path",
      "http://192.168.0.1:80/router/admin",
  };
  return urls;
}

static void BM_UrlAggregatorParse(benchmark::State& state) {
  const auto& urls = RealisticUrls();
  size_t success = 0;
  for (auto _ : state) {
    for (auto url : urls) {
      auto result = ada::parse<ada::url_aggregator>(url);
      success += result.has_value();
      benchmark::DoNotOptimize(result);
    }
  }
  benchmark::DoNotOptimize(success);
  state.SetItemsProcessed(state.iterations() * urls.size());
}
BENCHMARK(BM_UrlAggregatorParse);

static void BM_UrlParse(benchmark::State& state) {
  const auto& urls = RealisticUrls();
  for (auto _ : state) {
    for (auto url : urls) {
      auto result = ada::parse<ada::url>(url);
      benchmark::DoNotOptimize(result);
    }
  }
  state.SetItemsProcessed(state.iterations() * urls.size());
}
BENCHMARK(BM_UrlParse);

static void BM_UrlCanParse(benchmark::State& state) {
  const auto& urls = RealisticUrls();
  for (auto _ : state) {
    for (auto url : urls) {
      benchmark::DoNotOptimize(ada::can_parse(url));
    }
  }
  state.SetItemsProcessed(state.iterations() * urls.size());
}
BENCHMARK(BM_UrlCanParse);

static void BM_UrlParseWithBase(benchmark::State& state) {
  const std::vector<std::string_view> relative = {
      "../lib/index.js", "./package.json", "/absolute/path?x=1",
      "?only=query",     "#only-hash",     "//other.host/path",
      "sub/dir/file.mjs"};
  auto base = ada::parse<ada::url_aggregator>(
      "file:///home/user/projects/app/src/main.js");
  for (auto _ : state) {
    for (auto input : relative) {
      auto result = ada::parse<ada::url_aggregator>(input, &*base);
      benchmark::DoNotOptimize(result);
    }
  }
  state.SetItemsProcessed(state.iterations() * relative.size());
}
BENCHMARK(BM_UrlParseWithBase);

static void BM_UrlSetters(benchmark::State& state) {
  auto base = ada::parse<ada::url_aggregator>(
      "https://user:pass@example.com:8080/path?query=1#hash");
  for (auto _ : state) {
    ada::url_aggregator url = *base;
    url.set_protocol("http:");
    url.set_hostname("nodejs.org");
    url.set_port("3000");
    url.set_pathname("/api/v1/resource");
    url.set_search("?a=1&b=2");
    url.set_hash("#section");
    benchmark::DoNotOptimize(url.get_href());
  }
}
BENCHMARK(BM_UrlSetters);

static void BM_UrlSearchParams(benchmark::State& state) {
  const std::string query =
      "q=node.js&page=2&limit=50&sort=desc&filter=a%20b&filter=c%2Bd"
      "&utf8=%E2%9C%93&empty=&flag&encoded=%3Cscript%3E";
  for (auto _ : state) {
    ada::url_search_params params(query);
    params.append("extra", "value with spaces");
    params.sort();
    benchmark::DoNotOptimize(params.to_string());
  }
}
BENCHMARK(BM_UrlSearchParams);

static void BM_HrefFromFile(benchmark::State& state) {
  const std::vector<std::string_view> paths = {
      "/home/user/projects/app/node_modules/lodash/index.js",
      "/tmp/some file with spaces.txt",
      "/var/lib/data/#hash?query%percent.json"};
  for (auto _ : state) {
    for (auto path : paths) {
      benchmark::DoNotOptimize(ada::href_from_file(path));
    }
  }
}
BENCHMARK(BM_HrefFromFile);

BENCHMARK_MAIN();
