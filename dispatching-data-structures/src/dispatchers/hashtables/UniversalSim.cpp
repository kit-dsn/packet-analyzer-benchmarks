#include "dispatchers/hashtables/UniversalSim.h"

UniversalSim::UniversalSim() : a(0), b(0), wMinusM(0), generator(rd()) {
    setBins(2);

    table = std::vector<Value>(ONE << M, Value(0, nullptr));

    // Initialize random engine
    distributionA = std::uniform_int_distribution<uint64_t>(1, static_cast<word_t>(~static_cast<word_t>(0)));
    distributionB = std::uniform_int_distribution<uint64_t>(0, (ONE << wMinusM) - ONE);

    // Initialize random parameters
    randomizeAB();

    // Debug
    #if DEBUG > 0
    allCounter = 0;
    nptrCounter = 0;
    mismatchCounter = 0;
    #endif

    worked = 0;
    didntWork = 0;
}

UniversalSim::~UniversalSim() {
    freeAnalyzers();

    #if DEBUG > 0
    std::cout << "Nullptr hits: " << nptrCounter << "/" << allCounter << std::endl;
    std::cout << "Mismatch hits: " << mismatchCounter << "/" << allCounter << std::endl;
    std::cout << "Correct hits: " << allCounter - mismatchCounter - nptrCounter << "/" << allCounter << std::endl;
    #endif
}

bool UniversalSim::registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) {
    #if DEBUG > 1
    std::shared_ptr<void> deferred(nullptr, [=](...){ std::cout << "Inserted " << identifier << std::endl; });
    #endif

    uint64_t hashedID = hash(identifier);
    if (table[hashedID].second == nullptr) {
        // Free bin, insert the value
        table[hashedID] = Value(identifier, make_analyzer());
        return true;
    } else if (table[hashedID].first != identifier) {
        // The bin is not empty, but the content isn't the to-be-inserted identifier --> resolve collision

        // Create intermediate representation with the new element in it, then rehash with that data
        std::vector<Value> intermediate = createIntermediate();
        intermediate.emplace_back(identifier, make_analyzer());

        // Try increasing the #bins until it works or it can't get any larger.
        rehash(intermediate);
        return true;
    }

    // Analyzer with this ID is already registered.
    return false;
}

void UniversalSim::registerAnalyzers(const std::map<identifier_t, analyzer_builder> &analyzer_builders) {
    // Analyzer already registered
    for (const auto &current : analyzer_builders) {
        if (table[hash(current.first)].second != nullptr) {
            throw std::invalid_argument("Analyzer " + std::to_string(current.first) + " already registered!");
        }
    }

    // Create intermediate representation of current analyzer set, then add all new ones
    std::vector<Value> intermediate = createIntermediate();
    for (const auto& current : analyzer_builders) {
        intermediate.emplace_back(current.first, current.second());
    }

    rehash(intermediate);
}

IAnalyzer * UniversalSim::lookup(identifier_t identifier) {
    uint64_t hashedID = hash(identifier);

    // The hashedID can't be larger than the number of bins
    assert(hashedID < table.size() && "Hashed ID is outside of the hash table range!");

    Value entry = table[hashedID];

    #if DEBUG > 0
    if (entry.second == nullptr) {
        nptrCounter++;
    } else if (entry.first != identifier) {
        mismatchCounter++;
    }
    allCounter++;
    #endif

    if (entry.second != nullptr && entry.first == identifier) {
        return entry.second;
    }
    return nullptr;
}

size_t UniversalSim::size() {
    size_t result = 0;
    for (const auto& current : table) {
        if (current.second != nullptr) {
            result++;
        }
    }
    return result;
}

void UniversalSim::clear() {
    // Clear analyzer pointers
    freeAnalyzers();

    setBins(2);
    table = std::vector<Value>(ONE << M, Value(0, nullptr));
    randomizeAB();
}

size_t UniversalSim::bucketCount() {
    return table.size();
}

void UniversalSim::rehash() {
    // Intermediate representation is just the current table without nulls
    rehash(createIntermediate());
}

// #######################
// ####### PRIVATE #######
// #######################

void UniversalSim::stringifyAnalyzersState(std::ostream &os) const {
    for (const auto &current : table) {
        if (current.second != nullptr) {
            os << *current.second << "\n";
        }
    }
}

void UniversalSim::freeAnalyzers() {
    for (auto& current : table) {
        delete current.second;
        current.second = nullptr;
    }
}

void UniversalSim::rehash(const std::vector<Value>& intermediate) {
    while (!findCollisionFreeHashFunction(intermediate)) {
        #if DEBUG > 0
        std::cout << "Rehashing did not work. Increasing #bins to " << static_cast<uint64_t>(std::pow(2, M + 1));
        std::cout << " (" << M + 1 << "bit)." << std::endl;
        #endif

        setBins(M + 1);
    }
}

bool UniversalSim::findCollisionFreeHashFunction(const std::vector<Value>& intermediate) {
    // Don't even try if the number of values is larger than the number of buckets
    if (ONE << M < intermediate.size()) {
        return false;
    }

    // Remember the hash function parameters to not break the table if rehashing doesn't work
    uint64_t stored_a = a;
    uint64_t stored_b = b;

    // Because the hash function hashes all values in the universe uniformly to m bins with probability 1/m
    // we should at least try a multiple of #bins times.
    for (size_t i = 1; i <= 10'000'000; i++) {
        // Step 1: Re-randomize hash function parameters
        randomizeAB();

        // Step 2: Create new table
        std::vector<Value> newTable(ONE << M, Value(0, nullptr));

        // Step 3: Try to insert all elements into the new table with the new hash function
        bool finished = true;
        for (const auto &current : intermediate) {
            uint64_t hashedID = hash(current.first);
            if (newTable[hashedID].second == nullptr) {
                // Free bin, insert the value
                newTable[hashedID] = current;
            } else {
                // The bin is not empty which means there is a collision
                // (there are no duplicates in the intermediate representation so that can't be the case)
                finished = false;
//                std::cout << a << " * " << current.first << " + " << b << " >> " << static_cast<uint64_t>(wMinusM) << std::endl;
//                std::cout << "collides with" << std::endl;
//                std::cout << a << " * " << newTable[hashedID].first << " + " << b << " >> " << static_cast<uint64_t>(wMinusM) << std::endl;
//                std::cout << "Hash value: " << hashedID << std::endl;
//                std::cout << "Hash value: " << hash(current.first) << std::endl;
//                std::cout << "Hash value: " << hash(newTable[hashedID].first) << std::endl;
//                exit(1);
                #if DEBUG > 0
                static bool flag;
                if (flag && i % 100 == 0) {
                    std::cout << "Collision of " << current.first << " and " << newTable[hashedID].first
                              << " - hashcode is " << hashedID << "." << std::endl;
                }
                #endif
                break;
            }
        }

        // Step 4: If the inserting finished without collisions, overwrite the previous table and exit
        if (finished) {
            table = newTable;
            worked++;
        } else {
            didntWork++;
        }

        #if DEBUG > 0
        if (i % 10000 == 0) std::cout << "At iteration " << i << "\r" << std::flush;
        #endif
    }

    // Finding a collision free hash function failed. Revert the hash function parameters.
    a = stored_a;
    b = stored_b;
    return false;
}

size_t UniversalSim::real_size() {
    return table.size() * 2;
}
