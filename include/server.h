#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include "kv_store.h"
#include "thread_pool.h"

namespace kvstore {

class TcpServer {
public:
    TcpServer(std::uint16_t port,
              std::size_t workerThreads,
              std::size_t shardCount,
              const std::string& walPath);

    void run();
    void stop();

private:
    void handleClient(int clientSocket);

    std::uint16_t port_;
    ThreadPool pool_;
    KvStore store_;
    std::atomic<bool> stop_{false};
    int listenSocket_ = -1;
};

} // namespace kvstore
