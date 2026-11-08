// Benchmarks for merve, the lexer that detects CommonJS named exports when a
// CommonJS module is imported from ESM.

#include <benchmark/benchmark.h>

#include <string>

#include "merve.h"

// A typical transpiled (TypeScript/Babel style) CommonJS module.
static const char kTranspiledModule[] = R"JS(
"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    Object.defineProperty(o, k2, { enumerable: true, get: function() { return m[k]; } });
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __exportStar = (this && this.__exportStar) || function(m, exports) {
    for (var p in m) if (p !== "default" && !exports.hasOwnProperty(p)) __createBinding(exports, m, p);
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.parse = exports.stringify = exports.VERSION = void 0;
const path_1 = require("path");
const util_1 = require("./util");
/**
 * Parses the given input string.
 * @param {string} input
 */
function parse(input, options = {}) {
    const re = /^([a-z]+):\/\/([^/]+)(\/.*)?$/i;
    const match = re.exec(input);
    if (!match) {
        throw new Error(`Invalid input: ${input}`);
    }
    return { protocol: match[1], host: match[2], path: match[3] || '/' };
}
exports.parse = parse;
function stringify(obj) {
    return `${obj.protocol}://${obj.host}${path_1.posix.normalize(obj.path)}`;
}
exports.stringify = stringify;
exports.VERSION = '1.2.3';
__exportStar(require("./types"), exports);
__exportStar(require("./helpers"), exports);
)JS";

static const char kObjectLiteralExports[] = R"JS(
'use strict';
const a = require('./a');
function helper(x) { return x * 2 / 3; }
const template = `value: ${helper(1)} and ${'nested ' + `${a}`}`;
module.exports = {
  alpha, beta, gamma, delta, epsilon,
  zeta: helper,
  'eta': 1,
  theta: function () {},
  ...require('./reexported'),
};
)JS";

static std::string LargeModule() {
  std::string src = "'use strict';\n";
  for (int i = 0; i < 500; i++) {
    std::string n = std::to_string(i);
    src += "function fn" + n + "(a, b) {\n";
    src += "  // comment " + n + " with 'quotes' and /regex-like/ text\n";
    src += "  const s = `tmpl ${a + b} " + n + "`;\n";
    src += "  if (a / 2 > b) { return /ab+c/g.test(s); }\n";
    src += "  return { a, b, s, arr: [1, 2, 3].map((x) => x * " + n + ") };\n";
    src += "}\n";
    src += "exports.fn" + n + " = fn" + n + ";\n";
  }
  src += "module.exports.extra = require('./extra');\n";
  return src;
}

static void RunLexer(benchmark::State& state, const std::string& source) {
  for (auto _ : state) {
    auto result = lexer::parse_commonjs(source);
    if (!result) {
      state.SkipWithError("parse_commonjs failed");
      break;
    }
    benchmark::DoNotOptimize(result);
  }
  state.SetBytesProcessed(state.iterations() * source.size());
}

static void BM_LexTranspiledModule(benchmark::State& state) {
  RunLexer(state, kTranspiledModule);
}
BENCHMARK(BM_LexTranspiledModule);

static void BM_LexObjectLiteralExports(benchmark::State& state) {
  RunLexer(state, kObjectLiteralExports);
}
BENCHMARK(BM_LexObjectLiteralExports);

static void BM_LexLargeModule(benchmark::State& state) {
  RunLexer(state, LargeModule());
}
BENCHMARK(BM_LexLargeModule);

BENCHMARK_MAIN();
