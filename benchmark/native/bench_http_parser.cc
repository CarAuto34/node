// Benchmarks for llhttp, the HTTP/1.x parser behind the `http` module.

#include <benchmark/benchmark.h>

#include <cstring>
#include <string>

#include "llhttp.h"

struct Counters {
  size_t headers = 0;
  size_t body_bytes = 0;
  size_t messages = 0;
};

static int OnHeaderField(llhttp_t* parser, const char*, size_t) {
  static_cast<Counters*>(parser->data)->headers++;
  return 0;
}

static int OnBody(llhttp_t* parser, const char*, size_t length) {
  static_cast<Counters*>(parser->data)->body_bytes += length;
  return 0;
}

static int OnMessageComplete(llhttp_t* parser) {
  static_cast<Counters*>(parser->data)->messages++;
  return 0;
}

static int OnData(llhttp_t*, const char*, size_t) { return 0; }

static void InitSettings(llhttp_settings_t* settings) {
  llhttp_settings_init(settings);
  settings->on_url = OnData;
  settings->on_header_field = OnHeaderField;
  settings->on_header_value = OnData;
  settings->on_body = OnBody;
  settings->on_message_complete = OnMessageComplete;
}

static void RunParser(benchmark::State& state, llhttp_type_t type,
               const std::string& input) {
  llhttp_settings_t settings;
  InitSettings(&settings);
  Counters counters;
  for (auto _ : state) {
    llhttp_t parser;
    llhttp_init(&parser, type, &settings);
    parser.data = &counters;
    llhttp_errno_t err = llhttp_execute(&parser, input.data(), input.size());
    if (err != HPE_OK) {
      state.SkipWithError(llhttp_errno_name(err));
      break;
    }
    benchmark::DoNotOptimize(counters);
  }
  state.SetBytesProcessed(state.iterations() * input.size());
}

static const char kSimpleGet[] =
    "GET /favicon.ico HTTP/1.1\r\n"
    "Host: 0.0.0.0=5000\r\n"
    "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) "
    "Gecko/2008061015 Firefox/3.0\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
    "Accept-Language: en-us,en;q=0.5\r\n"
    "Accept-Encoding: gzip,deflate\r\n"
    "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
    "Keep-Alive: 300\r\n"
    "Connection: keep-alive\r\n"
    "\r\n";

static void BM_ParseRequest(benchmark::State& state) {
  RunParser(state, HTTP_REQUEST, kSimpleGet);
}
BENCHMARK(BM_ParseRequest);

static void BM_ParsePipelinedRequests(benchmark::State& state) {
  std::string input;
  for (int i = 0; i < 64; i++) input += kSimpleGet;
  RunParser(state, HTTP_REQUEST, input);
}
BENCHMARK(BM_ParsePipelinedRequests);

static void BM_ParsePostWithBody(benchmark::State& state) {
  const std::string body(static_cast<size_t>(state.range(0)), 'x');
  std::string input =
      "POST /api/v1/upload HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: " +
      std::to_string(body.size()) + "\r\n\r\n" + body;
  RunParser(state, HTTP_REQUEST, input);
}
BENCHMARK(BM_ParsePostWithBody)->Arg(4096);

static void BM_ParseChunkedResponse(benchmark::State& state) {
  std::string input =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/plain\r\n"
      "Transfer-Encoding: chunked\r\n"
      "Date: Tue, 04 Aug 2009 07:59:32 GMT\r\n"
      "Server: Node.js\r\n\r\n";
  const std::string chunk(256, 'y');
  for (int i = 0; i < 32; i++) {
    input += "100\r\n" + chunk + "\r\n";
  }
  input += "0\r\n\r\n";
  RunParser(state, HTTP_RESPONSE, input);
}
BENCHMARK(BM_ParseChunkedResponse);

static void BM_ParseManyHeaders(benchmark::State& state) {
  std::string input = "GET /resource HTTP/1.1\r\nHost: example.com\r\n";
  for (int i = 0; i < 100; i++) {
    input += "X-Custom-Header-" + std::to_string(i) +
             ": some-reasonably-long-header-value-" + std::to_string(i) +
             "\r\n";
  }
  input += "\r\n";
  RunParser(state, HTTP_REQUEST, input);
}
BENCHMARK(BM_ParseManyHeaders);

BENCHMARK_MAIN();
