#include "server.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace kvstore {

#ifdef _WIN32
using Socket = SOCKET;
const Socket kInvalidSocket = INVALID_SOCKET;
#else
using Socket = int;
const Socket kInvalidSocket = -1;
#endif

static void closeSocket(Socket sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

static bool initSockets() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

static void cleanupSockets() {
#ifdef _WIN32
    WSACleanup();
#endif
}

static bool sendAll(Socket sock, const std::string& data) {
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

static std::string trim(const std::string& input) {
    std::size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }
    std::size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }
    return input.substr(start, end - start);
}

static bool readLine(Socket sock, std::string* line) {
    line->clear();
    char ch = 0;
    while (true) {
        int received = recv(sock, &ch, 1, 0);
        if (received <= 0) {
            return false;
        }
        if (ch == '\n') {
            return true;
        }
        line->push_back(ch);
    }
}

TcpServer::TcpServer(std::uint16_t port,
                     std::size_t workerThreads,
                     std::size_t shardCount,
                     const std::string& walPath)
    : port_(port),
      pool_(workerThreads),
      store_(shardCount, walPath.empty() ? nullptr : std::make_unique<Wal>(walPath)) {}

void TcpServer::run() {
    if (!initSockets()) {
        throw std::runtime_error("socket init failed");
    }

    Socket listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == kInvalidSocket) {
        cleanupSockets();
        throw std::runtime_error("failed to create socket");
    }

    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    if (bind(listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        closeSocket(listenSock);
        cleanupSockets();
        throw std::runtime_error("bind failed");
    }

    if (listen(listenSock, SOMAXCONN) != 0) {
        closeSocket(listenSock);
        cleanupSockets();
        throw std::runtime_error("listen failed");
    }

    listenSocket_ = static_cast<int>(listenSock);

    while (!stop_.load()) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        Socket clientSock = accept(listenSock, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientSock == kInvalidSocket) {
            if (stop_.load()) {
                break;
            }
            continue;
        }

        pool_.enqueue([this, clientSock]() { handleClient(static_cast<int>(clientSock)); });
    }

    closeSocket(listenSock);
    cleanupSockets();
}

void TcpServer::stop() {
    stop_.store(true);
    if (listenSocket_ != -1) {
        closeSocket(static_cast<Socket>(listenSocket_));
    }
}

void TcpServer::handleClient(int clientSocket) {
    Socket sock = static_cast<Socket>(clientSocket);
    std::string line;
    while (readLine(sock, &line)) {
        std::string request = trim(line);
        if (request.empty()) {
            continue;
        }

        std::istringstream stream(request);
        std::string command;
        stream >> command;
        std::string response;

        if (command == "GET") {
            std::string key;
            stream >> key;
            if (key.empty()) {
                response = "ERROR missing key\n";
            } else {
                std::string value;
                if (store_.get(key, &value)) {
                    response = "VALUE " + value + "\n";
                } else {
                    response = "NOT_FOUND\n";
                }
            }
        } else if (command == "PUT") {
            std::string key;
            stream >> key;
            std::string value;
            std::getline(stream, value);
            value = trim(value);
            if (key.empty()) {
                response = "ERROR missing key\n";
            } else {
                store_.put(key, value);
                response = "OK\n";
            }
        } else if (command == "DEL") {
            std::string key;
            stream >> key;
            if (key.empty()) {
                response = "ERROR missing key\n";
            } else {
                response = store_.del(key) ? "OK\n" : "NOT_FOUND\n";
            }
        } else {
            response = "ERROR unknown command\n";
        }

        if (!sendAll(sock, response)) {
            break;
        }
    }
    closeSocket(sock);
}

} // namespace kvstore
