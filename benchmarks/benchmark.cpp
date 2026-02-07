#include "kv_store.h"

#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

using kvstore::KvStore;

namespace {

std::string randomKey(std::mt19937& rng) {
    std::uniform_int_distribution<int> dist(0, 9999);
    return "key" + std::to_string(dist(rng));
}

} // namespace

int main() {
    const std::size_t threadCount = std::max<std::size_t>(1, std::thread::hardware_concurrency());
    const std::size_t opsPerThread = 200000;

    KvStore store(16, nullptr);

    auto start = std::chrono::steady_clock::now();
    std::vector<std::thread> threads;

    for (std::size_t t = 0; t < threadCount; ++t) {
        threads.emplace_back([&store, opsPerThread, seed = static_cast<unsigned int>(t + 1)]() {
            std::mt19937 rng(seed);
            for (std::size_t i = 0; i < opsPerThread; ++i) {
                auto key = randomKey(rng);
                if (i % 3 == 0) {
                    store.put(key, "value");
                } else if (i % 3 == 1) {
                    std::string value;
                    store.get(key, &value);
                } else {
                    store.del(key);
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double totalOps = static_cast<double>(threadCount * opsPerThread);
    double opsPerSec = (totalOps / static_cast<double>(elapsed)) * 1000.0;

    std::cout << "Threads: " << threadCount << "\n";
    std::cout << "Ops: " << totalOps << "\n";
    std::cout << "Elapsed ms: " << elapsed << "\n";
    std::cout << "Throughput ops/sec: " << static_cast<long long>(opsPerSec) << "\n";
    return 0;
}
