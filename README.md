# Concurrent Key-Value Store

A production-grade, high-throughput in-memory key-value store built in C++17. Designed for ultra-low latency distributed systems, real-time caching layers, and high-concurrency workloads requiring sub-100µs access patterns.

## Technology Stack

| Layer | Technology |
|-------|-----------|
| **Language** | C++17 |
| **Concurrency** | Multi-threaded with fine-grained locking |
| **Storage** | Sharded hash tables (unordered_map) |
| **Durability** | Write-Ahead Log (WAL) |
| **Network** | TCP/IP with thread pool |
| **Build System** | CMake 3.16+ |
| **Platforms** | Windows (MSVC), Linux/Unix (GCC/Clang) |

## Use Cases

- **Real-Time Caching Layer**: Session storage, cache-aside pattern implementations for microservices
- **High-Frequency Trading Systems**: Sub-100µs latency requirements for order books and quote caches
- **IoT Data Aggregation**: Temporary storage for sensor streams before persistent storage
- **In-Memory Databases**: Embedded cache engine for analytics and ML pipelines
- **Distributed Systems**: Local cache node for multi-tier caching architectures

## Performance Metrics

### In-Memory Performance
**8 server threads, 16 shards, mixed workload (33% GET, 33% PUT, 34% DEL)**

| Metric | Value |
|--------|-------|
| Throughput | **3.87M ops/sec** |
| Total Operations | 4.4M |
| Execution Time | 1.138s |

### TCP Network Performance
**8 concurrent clients, 10K unique keys, 32B payloads, 5sec duration**

| Metric | Value |
|--------|-------|
| Throughput | **~119.6K ops/sec** |
| P50 Latency | 63 µs |
| P95 Latency | 102 µs |
| P99 Latency | 120 µs |
| Error Rate | 0% |

### Scalability
- **In-Memory**: 3.87M ops/sec (sharded)
- **Network**: 119.6K ops/sec per server instance
- **Linear Shard Scaling**: Throughput increases near-linearly with shard count
- **Sub-100µs Access Latency**: Consistent low-latency response times

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
