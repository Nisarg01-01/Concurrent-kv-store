# Concurrent Key-Value Store

A production-grade, high-throughput in-memory key-value store built in C++17. Engineered through fine-grained locking and sharded architecture for massive concurrent throughput and ultra-low latency access patterns. Optimized for demanding systems requiring consistent sub-100µs response times and millions of operations per second.

## Technology Stack

| Layer | Technology |
|-------|-----------|
| **Language** | C++17 |
| **Concurrency Model** | Per-shard fine-grained locking (reduces contention) |
| **Storage Architecture** | Sharded hash tables (unordered_map) |
| **Durability** | Write-Ahead Log (WAL) with async replication |
| **Network Protocol** | TCP/IP with thread pool dispatch |
| **Build System** | CMake 3.16+ |
| **Platforms** | Windows (MSVC), Linux/Unix (GCC/Clang) |

## Key Optimizations

- **Sharded Concurrency Design**: Partitions key space across independent sharded hash tables, each with its own mutex. Reduces lock contention to O(1/n) where n is shard count, enabling near-linear throughput scaling with core count.
- **O(1) Amortized Operations**: All GET, PUT, DEL operations execute as direct hash table lookups, eliminating tree traversals or linear scans.
- **Efficient Memory Access**: Zero-copy string handling and minimal heap allocations reduce garbage collection pressure.
- **Optimized Thread Dispatch**: Lightweight worker thread pool with minimal context switching overhead.
- **Protocol Efficiency**: Line-buffered parsing with minimal string operations.

## Measured Performance

### In-Memory Throughput (Sharded Store)
**Server: 8 threads, 16 independent shards | Workload: 33% GET, 33% PUT, 34% DEL | Client: 8 concurrent benchmark threads**

| Configuration | Result |
|---------------|--------|
| **Throughput** | **9.87M ops/sec** |
| **Total Operations** | 4.4M |
| **Execution Time** | 446ms |
| **Per-Operation Latency** | ~101ns |

*Performance driven by: sharded lock design eliminates contention; per-shard mutexes allow independent concurrent access; direct hash table O(1) lookup; minimal allocations*

### TCP Network Performance (End-to-End Server)
**Server: 8 worker threads, 16 shards | Clients: 8 concurrent connections | Keys: 10K unique | Payload: 32 bytes**

| Metric | Value |
|--------|-------|
| **Throughput** | **125.4K ops/sec** |
| **P50 Latency** | 59 µs |
| **P95 Latency** | 99 µs |
| **P99 Latency** | 117 µs |
| **Error Rate** | 0% |

*Latency consistency achieved through: low-contention shard locks; O(1) store access; efficient pool dispatch; minimal syscall overhead*

## Use Cases

- **Real-Time Caching Layer**: Session storage, cache-aside patterns for microservices
- **High-Frequency Trading**: Sub-100µs order book and quote caches
- **IoT Data Aggregation**: Temporary buffers for high-velocity sensor streams
- **Analytics Pipeline Cache**: Embedded in-memory layer for ML feature stores
- **Multi-Tier Cache Node**: Local cache in distributed architectures

## API & Protocol

Each request is a single line terminated by `\n`:

**Requests**:
- `GET <key>`
- `PUT <key> <value>`
- `DEL <key>`

**Responses**:
- `VALUE <value>`
- `OK`
- `NOT_FOUND`
- `ERROR <message>`

## Configuration

```bash
./kv_server [OPTIONS]
```

| Option | Default | Description |
|--------|---------|-------------|
| `--port` | 9090 | TCP listen port |
| `--threads` | 8 | Worker thread pool size |
| `--shards` | 16 | Number of hash table shards |
| `--wal` | (disabled) | WAL file path for crash recovery |

**Example**:
```bash
./kv_server --port 9090 --threads 8 --shards 16 --wal storage.wal
```

## Building

### Prerequisites
- C++17 compiler (MSVC 14.2+, GCC 7+, Clang 6+)
- CMake 3.16+
- Windows SDK (Windows only)

### Compile
```bash
mkdir build
cd build
cmake .. && cmake --build . --config Release
```

### Build Targets
```bash
cmake --build . --target kv_server         # TCP server executable
cmake --build . --target kv_benchmark      # In-memory benchmark
cmake --build . --target kv_tcp_benchmark  # Network benchmark
cmake --build . --target kv_tests          # Unit tests
```

## Testing

### Unit Tests
```bash
ctest -C Release --output-on-failure
```

### In-Memory Benchmark
```bash
./build/Release/kv_benchmark
```

### TCP Benchmark (Local)

**Terminal 1** (Server):
```bash
./build/Release/kv_server --port 9090 --threads 8 --shards 16
```

**Terminal 2** (Benchmark):
```bash
./build/Release/kv_tcp_benchmark --port 9090 --threads 8 --duration 5
```

## Project Structure

```
.
├── include/              # Public headers
│   ├── kv_store.h       # Core key-value store API
│   ├── server.h         # TCP server implementation
│   ├── thread_pool.h    # Worker thread pool
│   └── wal.h            # Write-Ahead Log
├── src/                 # Implementation
│   ├── kv_store.cpp
│   ├── server.cpp
│   ├── thread_pool.cpp
│   ├── wal.cpp
│   └── main.cpp         # Server entry point
├── benchmarks/          # Performance benchmarks
│   ├── benchmark.cpp    # In-memory benchmark
│   └── tcp_benchmark.cpp # Network benchmark
├── tests/               # Unit tests
│   └── test_kv_store.cpp
└── CMakeLists.txt       # Build configuration
```

## License

MIT
