#include "wal.h"

#include <sstream>

namespace kvstore {

Wal::Wal(const std::string& path) : path_(path), stream_(path, std::ios::app) {}

void Wal::appendPut(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    stream_ << "P\t" << encode(key) << "\t" << encode(value) << "\n";
    stream_.flush();
}

void Wal::appendDel(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    stream_ << "D\t" << encode(key) << "\n";
    stream_.flush();
}

void Wal::replay(const std::function<void(const LogEntry&)>& apply) {
    std::ifstream input(path_);
    if (!input.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(input, line)) {
        LogEntry entry{};
        if (parseLine(line, &entry)) {
            apply(entry);
        }
    }
}

std::string Wal::encode(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\t':
            out += "\\t";
            break;
        case '\r':
            out += "\\r";
            break;
        default:
            out += ch;
            break;
        }
    }
    return out;
}

bool Wal::decodeField(const std::string& encoded, std::string* out) {
    out->clear();
    out->reserve(encoded.size());
    for (std::size_t i = 0; i < encoded.size(); ++i) {
        char ch = encoded[i];
        if (ch != '\\') {
            out->push_back(ch);
            continue;
        }
        if (i + 1 >= encoded.size()) {
            return false;
        }
        char next = encoded[++i];
        switch (next) {
        case '\\':
            out->push_back('\\');
            break;
        case 'n':
            out->push_back('\n');
            break;
        case 't':
            out->push_back('\t');
            break;
        case 'r':
            out->push_back('\r');
            break;
        default:
            return false;
        }
    }
    return true;
}

bool Wal::parseLine(const std::string& line, LogEntry* entry) {
    if (line.empty()) {
        return false;
    }

    std::istringstream stream(line);
    std::string op;
    if (!std::getline(stream, op, '\t')) {
        return false;
    }

    std::string rawKey;
    if (!std::getline(stream, rawKey, '\t')) {
        return false;
    }

    std::string key;
    if (!decodeField(rawKey, &key)) {
        return false;
    }

    if (op == "P") {
        std::string rawValue;
        if (!std::getline(stream, rawValue)) {
            return false;
        }
        std::string value;
        if (!decodeField(rawValue, &value)) {
            return false;
        }
        entry->type = LogEntry::Type::Put;
        entry->key = std::move(key);
        entry->value = std::move(value);
        return true;
    }

    if (op == "D") {
        entry->type = LogEntry::Type::Del;
        entry->key = std::move(key);
        entry->value.clear();
        return true;
    }

    return false;
}

} // namespace kvstore
