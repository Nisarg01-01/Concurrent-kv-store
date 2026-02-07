#pragma once

#include <functional>
#include <fstream>
#include <mutex>
#include <string>

namespace kvstore {

struct LogEntry {
    enum class Type { Put, Del } type;
    std::string key;
    std::string value;
};

class Wal {
public:
    explicit Wal(const std::string& path);

    void appendPut(const std::string& key, const std::string& value);
    void appendDel(const std::string& key);

    void replay(const std::function<void(const LogEntry&)>& apply);

private:
    std::string path_;
    std::mutex mutex_;
    std::ofstream stream_;

    static std::string encode(const std::string& value);
    static bool decodeField(const std::string& encoded, std::string* out);
    static bool parseLine(const std::string& line, LogEntry* entry);
};

} // namespace kvstore
