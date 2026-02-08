#include "server.h"

#include <cstdint>
#include <iostream>
#include <string>

namespace {

void printUsage(const char* exe) {
    std::cout << "Usage: " << exe << " [--port <port>] [--threads <n>] [--shards <n>] [--wal <path>]\n";
}

bool parseArg(const char* arg, const char* name) {
    return std::string(arg) == name;
}

} // namespace

int main(int argc, char** argv) {
    std::uint16_t port = 9090;
    std::size_t threads = 8;
    std::size_t shards = 16;
    std::string walPath;

    for (int i = 1; i < argc; ++i) {
        if (parseArg(argv[i], "--port") && i + 1 < argc) {
            port = static_cast<std::uint16_t>(std::stoi(argv[++i]));
        } else if (parseArg(argv[i], "--threads") && i + 1 < argc) {
            threads = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (parseArg(argv[i], "--shards") && i + 1 < argc) {
            shards = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (parseArg(argv[i], "--wal") && i + 1 < argc) {
            walPath = argv[++i];
        } else if (parseArg(argv[i], "--help")) {
            printUsage(argv[0]);
            return 0;
        } else {
            printUsage(argv[0]);
            return 1;
        }
    }

    if (threads == 0 || shards == 0) {
        std::cerr << "threads and shards must be > 0\n";
        return 1;
    }

    try {
        kvstore::TcpServer server(port, threads, shards, walPath);
        std::cout << "kv_server listening on port " << port << "\n";
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
