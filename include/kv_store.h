#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "wal.h"

namespace kvstore {

class KvStore {
public:
    KvStore(std::size_t shardCount, std::unique_ptr<Wal> wal);

    bool get(const std::string& key, std::string* value) const;
    void put(const std::string& key, const std::string& value);
    bool del(const std::string& key);

    std::size_t shardCount() const;

private:
    struct Shard {
        std::unordered_map<std::string, std::string> data;
        mutable std::mutex mutex;
    };

    std::vector<Shard> shards_;
    std::unique_ptr<Wal> wal_;

    std::size_t shardIndex(const std::string& key) const;
    void applyPut(const std::string& key, const std::string& value);
    bool applyDel(const std::string& key);
};

} // namespace kvstore
