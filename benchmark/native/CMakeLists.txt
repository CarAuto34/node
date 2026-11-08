cmake_minimum_required(VERSION 3.16)
project(node_native_benchmarks LANGUAGES C CXX)

# Micro-benchmarks for the native libraries that sit on Node.js hot paths
# (URL parsing, HTTP parsing, base64/hex, CommonJS export detection,
# perf_hooks histograms). They are built against the sources vendored in
# deps/ so they track exactly what Node.js ships, without requiring a full
# Node.js build.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(NODE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(NODE_DEPS ${NODE_ROOT}/deps)

include(FetchContent)
set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  google_benchmark
  GIT_REPOSITORY https://github.com/CodSpeedHQ/codspeed-cpp
  GIT_TAG v2.4.0
  SOURCE_SUBDIR google_benchmark
)
FetchContent_MakeAvailable(google_benchmark)

# simdutf (used by ada, as in the Node.js build)
add_library(node_dep_simdutf STATIC ${NODE_DEPS}/v8/third_party/simdutf/simdutf.cpp)
target_include_directories(node_dep_simdutf PUBLIC ${NODE_DEPS}/v8/third_party/simdutf)

# ada (WHATWG URL parser)
add_library(node_dep_ada STATIC ${NODE_DEPS}/ada/ada.cpp)
target_include_directories(node_dep_ada PUBLIC ${NODE_DEPS}/ada)
target_compile_definitions(node_dep_ada PUBLIC ADA_USE_SIMDUTF=1)
target_link_libraries(node_dep_ada PUBLIC node_dep_simdutf)

# llhttp (HTTP/1.x parser)
add_library(node_dep_llhttp STATIC
  ${NODE_DEPS}/llhttp/src/api.c
  ${NODE_DEPS}/llhttp/src/http.c
  ${NODE_DEPS}/llhttp/src/llhttp.c)
target_include_directories(node_dep_llhttp PUBLIC ${NODE_DEPS}/llhttp/include)

# nbytes (base64 / hex / byte swapping / string search)
add_library(node_dep_nbytes STATIC ${NODE_DEPS}/nbytes/src/nbytes.cpp)
target_include_directories(node_dep_nbytes PUBLIC ${NODE_DEPS}/nbytes/include)

# merve (CommonJS export lexer)
add_library(node_dep_merve STATIC ${NODE_DEPS}/merve/merve.cpp)
target_include_directories(node_dep_merve PUBLIC ${NODE_DEPS}/merve)

# hdr_histogram (perf_hooks histograms)
add_library(node_dep_histogram STATIC ${NODE_DEPS}/histogram/src/hdr_histogram.c)
target_include_directories(node_dep_histogram PUBLIC
  ${NODE_DEPS}/histogram/include
  ${NODE_DEPS}/histogram/src)
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
  target_link_libraries(node_dep_histogram PUBLIC ${MATH_LIBRARY})
endif()

function(node_native_benchmark name)
  add_executable(${name} ${name}.cc)
  target_link_libraries(${name} PRIVATE benchmark::benchmark ${ARGN})
endfunction()

node_native_benchmark(bench_url node_dep_ada)
node_native_benchmark(bench_http_parser node_dep_llhttp)
node_native_benchmark(bench_nbytes node_dep_nbytes)
node_native_benchmark(bench_cjs_lexer node_dep_merve)
node_native_benchmark(bench_histogram node_dep_histogram)
