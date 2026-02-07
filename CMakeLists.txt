cmake_minimum_required(VERSION 3.16)
project(ConcurrentKVStore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(kvstore
    src/kv_store.cpp
    src/wal.cpp
)

target_include_directories(kvstore PUBLIC include)

if (WIN32)
    target_link_libraries(kvstore ws2_32)
endif()

add_executable(kv_tests tests/test_kv_store.cpp)
target_link_libraries(kv_tests kvstore)

add_executable(kv_benchmark benchmarks/benchmark.cpp)
target_link_libraries(kv_benchmark kvstore)

enable_testing()
add_test(NAME kv_tests COMMAND kv_tests)
