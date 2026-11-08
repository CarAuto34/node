# Native benchmarks

Micro-benchmarks for the native libraries that sit on Node.js hot paths,
built directly from the sources vendored in `deps/` (no full Node.js build
required):

| Binary              | Library          | Node.js feature                                 |
| ------------------- | ---------------- | ----------------------------------------------- |
| `bench_url`         | `deps/ada`       | `URL`, `URL.canParse()`, `URLSearchParams`      |
| `bench_http_parser` | `deps/llhttp`    | `http` request/response parsing                 |
| `bench_nbytes`      | `deps/nbytes`    | `Buffer` base64/hex codecs, `swap*`, `indexOf`  |
| `bench_cjs_lexer`   | `deps/merve`     | CommonJS named-export detection for ESM imports |
| `bench_histogram`   | `deps/histogram` | `perf_hooks` histograms                         |

The benchmarks use [Google Benchmark](https://github.com/google/benchmark)
through the [CodSpeed](https://codspeed.io) compatibility layer and run in CI
via `.github/workflows/codspeed.yml`.

## Running locally

```bash
cmake -S benchmark/native -B benchmark/native/build -DCMAKE_BUILD_TYPE=Release
cmake --build benchmark/native/build -j
benchmark/native/run.sh
```

To measure with CodSpeed's CPU simulation instrument, configure with
`-DCODSPEED_MODE=simulation` and run through the
[CodSpeed CLI](https://codspeed.io/docs/cli):

```bash
codspeed run --mode simulation -- benchmark/native/run.sh
```
