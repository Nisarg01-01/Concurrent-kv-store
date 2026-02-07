#include "kv_store.h"

#include <functional>

namespace kvstore {

KvStore::KvStore(std::size_t shardCount, std::unique_ptr<Wal> wal)
    : shards_(shardCount), wal_(std::move(wal)) {
    if (wal_) {
        wal_->replay([this](const LogEntry& entry) {
            if (entry.type == LogEntry::Type::Put) {
                applyPut(entry.key, entry.value);
            } else if (entry.type == LogEntry::Type::Del) {
                applyDel(entry.key);
            }
        });
    }
}

bool KvStore::get(const std::string& key, std::string* value) const {
    auto& shard = shards_[shardIndex(key)];
    std::lock_guard<std::mutex> lock(shard.mutex);
    auto it = shard.data.find(key);
    if (it == shard.data.end()) {
        return false;
    }
    *value = it->second;
    return true;
}

void KvStore::put(const std::string& key, const std::string& value) {
    if (wal_) {
        wal_->appendPut(key, value);
    }
    applyPut(key, value);
}

bool KvStore::del(const std::string& key) {
    if (wal_) {
        wal_->appendDel(key);
    }
    return applyDel(key);
}

std::size_t KvStore::shardCount() const {
    return shards_.size();
}

std::size_t KvStore::shardIndex(const std::string& key) const {
    return std::hash<std::string>{}(key) % shards_.size();
}

void KvStore::applyPut(const std::string& key, const std::string& value) {
    auto& shard = shards_[shardIndex(key)];
    std::lock_guard<std::mutex> lock(shard.mutex);
    shard.data[key] = value;
}

bool KvStore::applyDel(const std::string& key) {
    auto& shard = shards_[shardIndex(key)];
    std::lock_guard<std::mutex> lock(shard.mutex);
    return shard.data.erase(key) > 0;
}

} // namespace kvstore
