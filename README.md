# Concurrent Key-Value Store (C++)

A production-oriented, in-memory key-value store in C++17. It uses sharded storage to reduce contention and an optional write-ahead log (WAL) for durability.

## Features
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
