#include "kv_store.h"

#include <cassert>
#include <filesystem>
#include <iostream>

using kvstore::KvStore;
using kvstore::Wal;

namespace {

std::string tempWalPath() {
    auto path = std::filesystem::temp_directory_path() / "kvstore_test.wal";
    return path.string();
}

void removeFile(const std::string& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

} // namespace

int main() {
    const std::string walPath = tempWalPath();
    removeFile(walPath);

    {
        KvStore store(4, std::make_unique<Wal>(walPath));
        store.put("alpha", "1");
        store.put("beta", "2");
        std::string value;
        assert(store.get("alpha", &value) && value == "1");
        assert(store.del("beta"));
        assert(!store.get("beta", &value));
    }

    {
        KvStore store(4, std::make_unique<Wal>(walPath));
        std::string value;
        assert(store.get("alpha", &value) && value == "1");
        assert(!store.get("beta", &value));
    }

    removeFile(walPath);
    std::cout << "All tests passed.\n";
    return 0;
}
