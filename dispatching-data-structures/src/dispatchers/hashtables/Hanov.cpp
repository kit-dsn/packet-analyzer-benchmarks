#include <algorithm>

#include "analyzers/All.h"
#include "dispatchers/hashtables/Hanov.h"

Hanov::~Hanov() {
    freeAnalyzers();
}

bool Hanov::registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) {
    // Analyzer already registered
    if (lookup(identifier) != nullptr) {
        return false;
    }

    // Merge new analyzer with existing ones and rehash
    std::unordered_map<identifier_t, IAnalyzer*> newAnalyzerList;
    for (auto &&current : values) {
        // Do not copy over dummy elements
        if (current.second != nullptr) {
            newAnalyzerList.emplace(std::move(current));
        }
    }
    // values now contains empty analyzer pointers, do not use until after createMPH ran!

    // Will definitely insert a new one because we checked if the identifier already exists in the hashtable
    // Also, we need to use [] to overwrite existing dummy elements
    newAnalyzerList[identifier] = make_analyzer();

    createMPH(std::move(newAnalyzerList));
    return true;
}

void Hanov::registerAnalyzers(const std::map<identifier_t, analyzer_builder> &analyzer_builders) {
    // Analyzer already registered
    for (const auto &current : analyzer_builders) {
        if (lookup(current.first) != nullptr) {
            throw std::invalid_argument("Analyzer already registered!");
        }
    }

    // Merge new analyzer with existing ones and rehash
    std::unordered_map<identifier_t, IAnalyzer*> newAnalyzerList;
    for (auto &&current : values) {
        // Do not copy over dummy elements
        if (current.second != nullptr) {
            newAnalyzerList.emplace(std::move(current));
        }
    }
    // values now contains empty analyzer pointers, do not use until after createMPH ran!

    // Will definitely insert a new one because we checked if the identifier already exists in the hashtable
    // Also, we need to use [] to overwrite existing dummy elements
    for (auto &current : analyzer_builders) {
        newAnalyzerList[current.first] = current.second();
    }

    createMPH(std::move(newAnalyzerList));
}


IAnalyzer * Hanov::lookup(identifier_t identifier) {
    if (empty) {
        return nullptr;
    }

    // No .at() needed because it automatically gets modded into range
    uint32_t d = intermediate[hash(first_d, identifier) % _size];
    const Value &result = values[hash(d, identifier) % _size];

    return result.first == identifier ? result.second : nullptr;
}

size_t Hanov::size() {
    return _size;
}

void Hanov::clear() {
    // Free analyzers
    freeAnalyzers();

    empty = true;
    first_d = 0;
    intermediate.clear();
    values.clear();
}

//*********************************
//*********** PRIVATE *************
//*********************************

void Hanov::createMPH(std::unordered_map<identifier_t, IAnalyzer*> &&analyzers) {
    size_t numAnalyzers = analyzers.size();
    if (numAnalyzers == 0) {
        throw std::invalid_argument("No analyzers given.");
    }

    /**********************************************
     * Step 1: Place all of the keys into buckets *
     **********************************************/

    // Create as many empty buckets as there are keys (N)
    std::vector<std::vector<uint32_t>> buckets(numAnalyzers);
    for (auto &current : buckets) {
        current = std::vector<uint32_t>();
    }

    // Initialize intermediate with N zeros
    intermediate = std::vector<uint32_t>(numAnalyzers);

    // Initialize values with N nullptr
    values = std::vector<Value>(numAnalyzers);

    // Places keys into buckets according to hash table
    for (const auto &current : analyzers) {
        buckets[hash(first_d, current.first) % numAnalyzers].push_back(current.first);
    }

    // Insert a dummy if numAnalyzers is a multiple of 2^n-1 or 2^n+1. This makes the intermediate hashing collide
    // unusually often. Not sure why but it is a pattern. Start at n=32.
    for (size_t n = UINT32_MAX; n > 0; n >>= 1u) {
        if (checkMultiple(n, numAnalyzers) || (n != UINT32_MAX && checkMultiple(n + 2, numAnalyzers))) {
            // Insert at the first empty position
            for (size_t x = 0; x < MAX_IDENTIFIERS; x++) {
                if (analyzers.count(x) == 0) {
                    analyzers.emplace(x, nullptr);

                    #if DEBUG > 0
                    std::cerr << "[" << numAnalyzers << "] Inserted dummy with key ";
                    PRINT_UINT_HEX(std::cerr, x, 8);
                    std::cerr << std::endl;
                    #endif

                    createMPH(std::move(analyzers));
                    return;
                }
            }
            throw std::length_error("Could not insert dummy: no free space found!");
        }
    }

    /******************************************************
    * Step 2: Sort buckets, process ordered by most items *
    *******************************************************/

    std::sort(buckets.begin(), buckets.end(), [](std::vector<uint32_t> &first, std::vector<uint32_t> &second) {
        return first.size() > second.size();
    });

    size_t b = 0;
    for (; b < numAnalyzers; b++) {
        const auto &bucket = buckets[b];

        // Stop if there are no more buckets with content
        if (bucket.empty()) {
            break;
        }

        uint32_t d = 1;
        uint32_t item = 0;
        std::vector<uint32_t> slots;
        while (item < bucket.size()) {
            uint32_t slot = hash(d, bucket[item]) % numAnalyzers;

            // If slot is already in use in general or by another element in this attempt, retry with different d
            if (values[slot].second != nullptr || std::find(slots.begin(), slots.end(), slot) != slots.end()) {
                d++;
                item = 0;
                slots.clear();

                if (d > 10'000'000) {
//                    for (const auto &current : bucket) {
//                        hashDebug(first_d, current, numAnalyzers);
//                    }

                    // The d for the first hash function doesn't work. Try another one.
                    if (first_d == 1'000) {
                        throw std::runtime_error("Couldn't find working first hash function.");
                    }
                    first_d++;

                    // Move back all analyzers that where already moved away
                    size_t count = 0;
                    for (auto &current : values) {
                        // Use [] instead of emplace to overwrite rests of moved-aways entries in "analyzers"
                        // Also, move only non-dummies.
                        // TODO: This might not be needed anymore as there is no std::move with raw pointers
                        if (current.second != nullptr) {
                            analyzers[current.first] = current.second;
                            count++;
                        }
                    }
                    #if DEBUG > 0
                    if (count > 0) {
                        std::cerr << "Moved back " << count << " values." << std::endl;
                    }
                    std::cerr << "[" << numAnalyzers << "] d value " << first_d << " for intermediate hashing didn't work. Trying another one... " << std::endl;
                    #endif

                    createMPH(std::move(analyzers));
                    return;
                }
            } else {
                slots.push_back(slot);
                item++;
            }
        }

        // Success. Remember working d in intermediate table for this key.
        // Uses bucket[0] because the hash is the same for all values in the bucket.
        intermediate[hash(first_d, bucket[0]) % numAnalyzers] = d;

        // Add all target elements into the values hashtable corresponding to their hash (remembered in order in slots)
        for (size_t i = 0; i < bucket.size(); i++) {
            values[slots[i]] = std::pair<uint32_t, IAnalyzer*>(bucket[i], analyzers[bucket[i]]);
        }
    }

    _size = values.size();
    empty = false;
}

void Hanov::stringifyAnalyzersState(std::ostream &os) const {
    for (size_t i = 0; i < values.size(); i++) {
        // Skip dummy keys
        if (values[i].second == nullptr) {
            continue;
        }

        os << "[KEY ";
        PRINT_UINT_HEX(os, values[i].first, 8);
        os << "] ";
        os << *values[i].second;
        if (i != values.size() - 1) {
            os << "\n";
        }
    }
}

void Hanov::freeAnalyzers() {
    for (auto &current : values) {
        delete current.second;
        current.second = nullptr;
    }
}

size_t Hanov::real_size() {
    // Ignore dummies
    size_t counter = 0;
    for (const auto& current : values) {
//        if (current.second != nullptr) {
        counter += 4 + 2 + 8; // 4 byte intermediate + the "Value" of 2 byte id and 8 byte analyzer pointer
//        }
    }
    return counter;
}
