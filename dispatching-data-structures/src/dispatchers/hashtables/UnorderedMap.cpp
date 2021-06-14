#include "dispatchers/hashtables/UnorderedMap.h"

UnorderedMap::~UnorderedMap() {
    freeAnalyzers();
}

bool UnorderedMap::registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) {
    if(!table.emplace(identifier, make_analyzer()).second) {
        return false;
    }

    // If there is a bucket collision, rehash
    while (containsBucketCollision()) {
        auto newSize = static_cast<size_t>(table.bucket_count() * 0.1);
        table.rehash(table.bucket_count() + (newSize > 0 ? newSize : 1));
        #if DEBUG > 0
        std::cout << "#buckets after rehashing: " << table.bucket_count() << std::endl;
        #endif
    }

    #if DEBUG > 0
    static size_t lastBucketCount;
    if (lastBucketCount != table.bucket_count()) {
        std::cout << "#buckets: " << table.bucket_count() << std::endl;
    }
    lastBucketCount = table.bucket_count();
    #endif
    return true;
}


void UnorderedMap::registerAnalyzers(const std::map<identifier_t, analyzer_builder> &analyzer_builders) {
    for (auto &current : analyzer_builders) {
        if (!table.emplace(current.first, current.second()).second) {
            throw std::invalid_argument("Analyzer already registered!");
        }

        #if DEBUG > 0
        static size_t lastBucketCount;
        if (lastBucketCount != table.bucket_count()) {
            std::cout << "#buckets: " << table.bucket_count() << std::endl;
        }
        lastBucketCount = table.bucket_count();
        #endif
    }

    // If there is a bucket collision, rehash
    while (containsBucketCollision()) {
        auto newSize = static_cast<size_t>(table.bucket_count() * 0.1);
        table.rehash(table.bucket_count() + (newSize > 0 ? newSize : 1));
        #if DEBUG > 0
        std::cout << "#buckets after rehashing: " << table.bucket_count() << std::endl;
        #endif
    }
}

IAnalyzer* UnorderedMap::lookup(identifier_t identifier) {
    if (table.count(identifier) != 0) {
        return table.at(identifier);
    } else {
        return nullptr;
    }
}

size_t UnorderedMap::size() {
    return table.size();
}

void UnorderedMap::clear() {
    freeAnalyzers();
    table.clear();
}

void UnorderedMap::stringifyAnalyzersState(std::ostream &os) const {
    for (const auto &current : table) {
        os << *current.second << "\n";
    }
}

void UnorderedMap::freeAnalyzers() {
    for (auto &current : table) {
        delete current.second;
        current.second = nullptr;
    }
}

size_t UnorderedMap::real_size() {
    // Every bucket is 8 byte analyzer + 2 byte identifier, both null if empty
    return table.bucket_count() * 10;
}
