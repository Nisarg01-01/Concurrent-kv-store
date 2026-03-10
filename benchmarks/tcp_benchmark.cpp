#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

namespace {

#ifdef _WIN32
using Socket = SOCKET;
const Socket kInvalidSocket = INVALID_SOCKET;
#else
using Socket = int;
const Socket kInvalidSocket = -1;
#endif

struct Options {
    std::string host = "127.0.0.1";
    std::uint16_t port = 9090;
    std::size_t threads = 8;
    std::size_t keyCount = 10000;
    std::size_t payloadSize = 32;
    int warmupSec = 2;
    int durationSec = 5;
};

struct Stats {
    std::vector<long long> latenciesUs;
    std::size_t ops = 0;
    std::size_t errors = 0;
};

bool parseArg(const char* arg, const char* name) {
    return std::string(arg) == name;
}

void printUsage(const char* exe) {
    std::cout << "Usage: " << exe
              << " [--host <ip>] [--port <port>] [--threads <n>] [--keys <n>]"
              << " [--payload <bytes>] [--warmup <sec>] [--duration <sec>]\n";
}

bool initSockets() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

void cleanupSockets() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void closeSocket(Socket sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

bool sendAll(Socket sock, const std::string& data) {
    std::size_t total = 0;
    while (total < data.size()) {
        int sent = send(sock, data.data() + total, static_cast<int>(data.size() - total), 0);
        if (sent <= 0) {
            return false;
        }
        total += static_cast<std::size_t>(sent);
    }
    return true;
}

class LineReader {
public:
    bool readLine(Socket sock, std::string* line) {
        line->clear();
        while (true) {
            auto pos = buffer_.find('\n');
            if (pos != std::string::npos) {
                *line = buffer_.substr(0, pos);
                buffer_.erase(0, pos + 1);
                return true;
            }

            char chunk[4096];
            int received = recv(sock, chunk, sizeof(chunk), 0);
            if (received <= 0) {
                return false;
            }
            buffer_.append(chunk, static_cast<std::size_t>(received));
        }
    }

private:
    std::string buffer_;
};

Socket connectTo(const Options& options) {
    Socket sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == kInvalidSocket) {
        return kInvalidSocket;
    }

    int flag = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(flag));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(options.port);

    if (inet_pton(AF_INET, options.host.c_str(), &addr.sin_addr) <= 0) {
        closeSocket(sock);
        return kInvalidSocket;
    }

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        closeSocket(sock);
        return kInvalidSocket;
    }

    return sock;
}

std::string makeValue(std::size_t payloadSize) {
    return std::string(payloadSize, 'x');
}

std::string makeKey(std::size_t index) {
    return "key" + std::to_string(index);
}

void worker(const Options& options, const std::string& value, Stats* stats) {
    Socket sock = connectTo(options);
    if (sock == kInvalidSocket) {
        stats->errors++;
        return;
    }

    LineReader reader;
    std::mt19937 rng(static_cast<unsigned int>(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<std::size_t> keyDist(0, options.keyCount - 1);
    std::uniform_int_distribution<int> opDist(0, 9);

    auto warmupEnd = std::chrono::steady_clock::now() + std::chrono::seconds(options.warmupSec);
    auto runEnd = warmupEnd + std::chrono::seconds(options.durationSec);

    auto sendOp = [&](const std::string& request) -> bool {
        if (!sendAll(sock, request)) {
            return false;
        }
        std::string line;
        return reader.readLine(sock, &line);
    };

    while (std::chrono::steady_clock::now() < warmupEnd) {
        std::size_t keyIndex = keyDist(rng);
        int op = opDist(rng);
        std::string request;
        if (op < 6) {
            request = "GET " + makeKey(keyIndex) + "\n";
        } else if (op < 9) {
            request = "PUT " + makeKey(keyIndex) + " " + value + "\n";
        } else {
            request = "DEL " + makeKey(keyIndex) + "\n";
        }
        if (!sendOp(request)) {
            stats->errors++;
            closeSocket(sock);
            return;
        }
    }

    while (std::chrono::steady_clock::now() < runEnd) {
        std::size_t keyIndex = keyDist(rng);
        int op = opDist(rng);
        std::string request;
        if (op < 6) {
            request = "GET " + makeKey(keyIndex) + "\n";
        } else if (op < 9) {
            request = "PUT " + makeKey(keyIndex) + " " + value + "\n";
        } else {
            request = "DEL " + makeKey(keyIndex) + "\n";
        }

        auto start = std::chrono::steady_clock::now();
        if (!sendOp(request)) {
            stats->errors++;
            break;
        }
        auto end = std::chrono::steady_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        stats->latenciesUs.push_back(us);
        stats->ops++;
    }

    closeSocket(sock);
}

long long percentile(const std::vector<long long>& data, double p) {
    if (data.empty()) {
        return 0;
    }
    double idx = (p / 100.0) * static_cast<double>(data.size() - 1);
    std::size_t i = static_cast<std::size_t>(idx);
    return data[i];
}

} // namespace

int main(int argc, char** argv) {
    Options options;

    for (int i = 1; i < argc; ++i) {
        if (parseArg(argv[i], "--host") && i + 1 < argc) {
            options.host = argv[++i];
        } else if (parseArg(argv[i], "--port") && i + 1 < argc) {
            options.port = static_cast<std::uint16_t>(std::stoi(argv[++i]));
        } else if (parseArg(argv[i], "--threads") && i + 1 < argc) {
            options.threads = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (parseArg(argv[i], "--keys") && i + 1 < argc) {
            options.keyCount = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (parseArg(argv[i], "--payload") && i + 1 < argc) {
            options.payloadSize = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (parseArg(argv[i], "--warmup") && i + 1 < argc) {
            options.warmupSec = std::stoi(argv[++i]);
        } else if (parseArg(argv[i], "--duration") && i + 1 < argc) {
            options.durationSec = std::stoi(argv[++i]);
        } else if (parseArg(argv[i], "--help")) {
            printUsage(argv[0]);
            return 0;
        } else {
            printUsage(argv[0]);
            return 1;
        }
    }

    if (options.threads == 0 || options.keyCount == 0 || options.durationSec <= 0) {
        std::cerr << "threads, keys, and duration must be > 0\n";
        return 1;
    }

    if (!initSockets()) {
        std::cerr << "socket init failed\n";
        return 1;
    }

    std::string value = makeValue(options.payloadSize);

    std::vector<Stats> stats(options.threads);
    std::vector<std::thread> workers;
    workers.reserve(options.threads);

    auto benchStart = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < options.threads; ++i) {
        workers.emplace_back(worker, std::cref(options), std::cref(value), &stats[i]);
    }

    for (auto& thread : workers) {
        thread.join();
    }

    auto benchEnd = std::chrono::steady_clock::now();
    auto totalElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(benchEnd - benchStart).count();

    std::vector<long long> allLatencies;
    std::size_t totalOps = 0;
    std::size_t totalErrors = 0;
    for (const auto& s : stats) {
        totalOps += s.ops;
        totalErrors += s.errors;
        allLatencies.insert(allLatencies.end(), s.latenciesUs.begin(), s.latenciesUs.end());
    }

    std::sort(allLatencies.begin(), allLatencies.end());

    double opsPerSec = 0.0;
    if (options.durationSec > 0) {
        opsPerSec = static_cast<double>(totalOps) / static_cast<double>(options.durationSec);
    }

    std::cout << "Server: " << options.host << ":" << options.port << "\n";
    std::cout << "Threads: " << options.threads << "\n";
    std::cout << "Keys: " << options.keyCount << "\n";
    std::cout << "Payload bytes: " << options.payloadSize << "\n";
    std::cout << "Warmup sec: " << options.warmupSec << "\n";
    std::cout << "Duration sec: " << options.durationSec << "\n";
    std::cout << "Total ops: " << totalOps << "\n";
    std::cout << "Errors: " << totalErrors << "\n";
    std::cout << "Throughput ops/sec: " << static_cast<long long>(opsPerSec) << "\n";

    if (!allLatencies.empty()) {
        std::cout << "Latency p50 us: " << percentile(allLatencies, 50.0) << "\n";
        std::cout << "Latency p95 us: " << percentile(allLatencies, 95.0) << "\n";
        std::cout << "Latency p99 us: " << percentile(allLatencies, 99.0) << "\n";
    }

    std::cout << "Elapsed ms: " << totalElapsed << "\n";

    cleanupSockets();
    return 0;
}
