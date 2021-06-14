cmake_minimum_required(VERSION 3.13)
project(Prototype)

set(CMAKE_CXX_STANDARD 17)
# Werror=return-type because clang produces a hard-to-detect SIGILL if the return is missing
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -pthread -DDEBUG=1 -Wall -Wvla -Wno-write-strings -Werror=return-type -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -pthread -DNDEBUG -Wall -Wvla -Wno-write-strings -Werror=return-type -Wpedantic")

find_package(PkgConfig REQUIRED)

include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

# Compile Cuckoo-hash
ExternalProject_Add(
    CuckooHash
    PREFIX ${CMAKE_BINARY_DIR}/cuckoo
    GIT_REPOSITORY https://github.com/kroki/Cuckoo-hash.git
    GIT_TAG 0.3
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR ${CMAKE_BINARY_DIR}/cuckoo/cuckoo-src
    CONFIGURE_COMMAND
        ${CMAKE_BINARY_DIR}/cuckoo/cuckoo-src/bootstrap.sh
        && ${CMAKE_BINARY_DIR}/cuckoo/cuckoo-src/configure
    BUILD_COMMAND $(MAKE)
    BINARY_DIR ${CMAKE_BINARY_DIR}/cuckoo/cuckoo-build
    INSTALL_COMMAND ""
)

# Compile Google Benchmark
ExternalProject_Add(
    GoogleBenchmark
    PREFIX ${CMAKE_BINARY_DIR}/gbench
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.5.2
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR ${CMAKE_BINARY_DIR}/gbench/gbench-src
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_GTEST_TESTS=OFF
    BUILD_COMMAND $(MAKE)
    BINARY_DIR ${CMAKE_BINARY_DIR}/gbench/gbench-build
    INSTALL_COMMAND ""
)

# Compile PAPI
ExternalProject_Add(
    PAPI
    PREFIX ${CMAKE_BINARY_DIR}/papi
    GIT_REPOSITORY https://bitbucket.org/icl/papi.git
    GIT_TAG stable-5.7
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR ${CMAKE_BINARY_DIR}/papi/papi-src
    CONFIGURE_COMMAND cd ${CMAKE_BINARY_DIR}/papi/papi-src/src && ./configure --prefix=${CMAKE_BINARY_DIR}/papi/papi-install
    BUILD_COMMAND cd ${CMAKE_BINARY_DIR}/papi/papi-src/src && $(MAKE)
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND cd ${CMAKE_BINARY_DIR}/papi/papi-src/src && $(MAKE) install
)

set(SRC
    InputReader.cpp
    MyPacket.cpp
    Timing.cpp

    analyzers/ETHAnalyzer.cpp
    analyzers/IPv4Analyzer.cpp
    analyzers/IPv6Analyzer.cpp
    analyzers/TCPAnalyzer.cpp
    analyzers/UDPAnalyzer.cpp
    analyzers/UnknownAnalyzer.cpp

    dispatchers/TreeMap.cpp

    dispatchers/hashtables/Array.cpp
    dispatchers/hashtables/Cuckoo.cpp
    dispatchers/hashtables/Hanov.cpp
    dispatchers/hashtables/Sparse.cpp
    dispatchers/hashtables/SparseUpper.cpp
    dispatchers/hashtables/Universal.cpp
    dispatchers/hashtables/UniversalSim.cpp
    dispatchers/hashtables/UnorderedMap.cpp
    dispatchers/hashtables/Vector.cpp

    dispatchers/metaprogramming/IMeta.cpp
    dispatchers/metaprogramming/GeneratedArrayFragmented.cpp
    dispatchers/metaprogramming/GeneratedArrayZeek.cpp
    dispatchers/metaprogramming/GeneratedIfFragmented.cpp
    dispatchers/metaprogramming/GeneratedIfZeek.cpp
    dispatchers/metaprogramming/GeneratedSwitchFragmented.cpp
    dispatchers/metaprogramming/GeneratedSwitchZeek.cpp
)
list(TRANSFORM SRC PREPEND src/)

# Build Benchmark
add_executable(benchmark ${SRC} src/benchmarkMain.cpp)
add_dependencies(benchmark CuckooHash GoogleBenchmark)
target_include_directories(benchmark PRIVATE
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_BINARY_DIR}/gbench/gbench-src/include
    ${CMAKE_BINARY_DIR}/cuckoo/cuckoo-src/src/
    ${CMAKE_BINARY_DIR}/papi/papi-install/include/
)
target_link_libraries(benchmark
    ${CMAKE_BINARY_DIR}/cuckoo/cuckoo-build/src/.libs/libcuckoo_hash.a
    ${CMAKE_BINARY_DIR}/gbench/gbench-build/src/libbenchmark.a
)

# Build CacheAnalyzer
add_executable(cache_analyzer ${SRC} src/cacheAnalyzerMain.cpp)
add_dependencies(cache_analyzer CuckooHash PAPI)
target_include_directories(cache_analyzer PRIVATE
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_BINARY_DIR}/cuckoo/cuckoo-src/src/
    ${CMAKE_BINARY_DIR}/papi/papi-install/include/
)
target_link_libraries(cache_analyzer
    ${CMAKE_BINARY_DIR}/cuckoo/cuckoo-build/src/.libs/libcuckoo_hash.a
    ${CMAKE_BINARY_DIR}/papi/papi-install/lib/libpapi.a
)
