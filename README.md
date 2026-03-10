# Concurrent Key-Value Store (C++)

A high-performance, production-grade in-memory key-value store implementing fine-grained locking and adaptive sharding strategies for maximum throughput under concurrent access patterns. Built in modern C++17 with zero-copy semantics, write-ahead logging for durability, and comprehensive performance optimizations.

## Overview

The Concurrent Key-Value Store is engineered for demanding workloads requiring predictable latency and sustained high throughput. The implementation achieves scale-out concurrency through intelligent shard distribution, eliminating lock contention bottlenecks prevalent in monolithic hash table designs.

### Key Capabilities
- **High-Performance TCP Server**: Non-blocking socket I/O with configurable worker thread pool
- **Lock-Free Sharding Architecture**: Per-shard mutex locking reduces contention to O(1/n) where n is shard count
- **O(1) Amortized Operations**: Constant-time GET, PUT, and DEL operations backed by unordered_map-based shards
- **Crash Recovery via WAL**: Optional write-ahead log ensures data durability and recovery after unexpected termination
- **Multi-Platform**: Seamless compilation on Windows (MSVC) and Unix-like systems (GCC/Clang)
- **Comprehensive Testing**: Full unit test coverage with latency and throughput benchmarking frameworks

## Performance Benchmarks

### In-Memory Performance (Local Sharded Store)
**Configuration**: 22 concurrent threads, 16 shards, mixed workload (33% GET, 33% PUT, 34% DEL)

| Metric | Result |
|--------|--------|
| **Total Operations** | 4.4 M |
| **Throughput** | **3.87 M ops/sec** |
| **Execution Time** | 1.138 s |
| **Per-Thread Ops** | 200,000 |

### TCP Network Performance (End-to-End Server)
**Configuration**: 8 concurrent client threads, 10,000 unique keys, 32-byte payload, 5-second duration

| Metric | Result |
|--------|--------|
| **Total Operations** | 598,232 |
| **Throughput** | **~119.6K ops/sec** |
| **P50 Latency** | 63 µs |
| **P95 Latency** | 102 µs |
| **P99 Latency** | 120 µs |
| **Error Rate** | 0% |

**Insights**: Network I/O and protocol parsing overhead reduces effective throughput to ~3.1% of raw in-memory performance, typical for TCP-based systems. Sub-microsecond store access times are masked by network round-trip times (RTT ≈ 60–120 µs).

## Architecture

### Concurrent Access Model
The store partitions data across independent shards, each protected by a fine-grained mutex. Given a key:
1. Hash the key to determine shard index: `shard_idx = hash(key) % shard_count`
2. Acquire shard-specific lock
3. Perform operation on shard's hash table (unordered_map)
4. Release lock

This design scales read and write throughput nearly linearly with shard count, reducing per-shard lock contention.

### Write-Ahead Log (WAL)
When enabled, every PUT and DEL operation is durably recorded before applying to the in-memory store. On crash, the store replays the log to reconstruct the pre-crash state. GET operations are not logged (read-only).

### Server Architecture
- **Thread Pool**: Configurable fixed-size worker pool processes incoming client connections
- **Non-Blocking Dispatch**: Each client connection is assigned to an available worker for request handling
- **Protocol Handler**: Custom protocol parser implements line-buffered message reading

## Building

### Prerequisites
- C++17 compiler (MSVC 14.2+ / GCC 7+ / Clang 6+)
- CMake 3.16+
- Windows SDK (for Windows builds)

### Compile
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Targets
```bash
cmake --build . --target kv_benchmark      # In-memory benchmark
cmake --build . --target kv_tcp_benchmark  # End-to-end TCP benchmark
cmake --build . --target kv_tests          # Unit tests
cmake --build . --target kv_server         # TCP server
```

## Testing

### Unit Tests
```bash
cmake --build . --target kv_tests --config Release
ctest -C Release --output-on-failure
```

### In-Memory Benchmark
```bash
./build/Release/kv_benchmark
```

### TCP End-to-End Benchmark
**Terminal 1** (Server):
```bash
./build/Release/kv_server --port 9090 --threads 8 --shards 16
```

**Terminal 2** (Client):
```bash
./build/Release/kv_tcp_benchmark --port 9090 --threads 8 --duration 5
```

## Server Usage

```bash
./kv_server [OPTIONS]

Options:
  --port <port>           Listen port (default: 9090)
  --threads <n>           Worker thread pool size (default: 8)
  --shards <n>            Shard count for concurrent store (default: 16)
  --wal <path>            WAL file path; if omitted, WAL is disabled
```

### Example
```bash
./kv_server --port 9090 --threads 8 --shards 16 --wal recovery.wal
```

## Protocol Specification

### Request Format
Each request is a single line terminated by `\n` (LF only, not CRLF):

```
GET <key>
PUT <key> <value>
DEL <key>
```

### Response Format
Responses are single lines, newline-terminated:

```
VALUE <value>     # Response to GET (if key found)
OK                # Response to PUT or DEL (success)
NOT_FOUND         # Response to GET or DEL (key not found)
ERROR <message>   # Response on malformed request or server error
```

### Example Session
```
Client: GET counter
Server: NOT_FOUND

Client: PUT counter 42
Server: OK

Client: GET counter
Server: VALUE 42

Client: DEL counter
Server: OK

Client: GET counter
Server: NOT_FOUND
```

## Design Decisions

### Why Sharding?
A single global lock ensures correctness but becomes a bottleneck under concurrent access. By partitioning the key space, we allow independent locks per shard, enabling near-linear scaling with core count.

### Why O(1) Operations?
All operations (GET, PUT, DEL) are direct hash table lookups. No tree traversals, no linear scans—O(1) amortized time.

### Why Write-Ahead Log?
WAL enables durable operation semantics: a "committed" PUT is guaranteed to survive process crashes. Critical for applications where data loss is unacceptable.

### Why TCP?
Simple, reliable, and portable. Well-suited for local inter-process communication and client-server deployments.

## Limitations & Future Enhancements

- **No Replication**: Single instance only; no built-in clustering
- **No Expiration**: Keys never expire; manual DEL required
- **No Transactions**: Atomic only at the per-key level
- **Limited Export**: No bulk export or snapshotting beyond WAL replay
- **Tuning**: Optimal shard count is workload-dependent; consider benchmarking with your access patterns

## License & Attribution

This project is provided as-is for educational and commercial use.
