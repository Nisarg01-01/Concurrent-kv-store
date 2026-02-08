# Concurrent Key-Value Store (C++)

A production-oriented, multithreaded in-memory key-value store in C++17. It uses a fixed-size thread pool, sharded storage to reduce contention, and an optional write-ahead log (WAL) for durability.

## Features
- Blocking TCP server with a thread pool
- Sharded in-memory storage for concurrent access
- O(1) GET/PUT/DEL operations (amortized)
- Optional WAL for crash recovery
- Simple unit tests and a micro-benchmark

## Build

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Tests

```bash
cmake --build . --target kv_tests
ctest
```

## Benchmark

```bash
cmake --build . --target kv_benchmark
./kv_benchmark
```

## Run

```bash
./kv_server --port 9090 --threads 8 --shards 16 --wal kv.wal
```

## Protocol
Each request is a single line terminated by `\n`:

- `GET <key>`
- `PUT <key> <value>`
- `DEL <key>`

Responses:
- `VALUE <value>`
- `OK`
- `NOT_FOUND`
- `ERROR <message>`
