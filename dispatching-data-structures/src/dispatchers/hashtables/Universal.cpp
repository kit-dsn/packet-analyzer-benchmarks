#include "dispatchers/hashtables/Universal.h"

Universal::Universal() : a(0), b(0), wMinusM(0), generator(rd()) {
    setBins(2);

    table = std::vector<Value>(ONE << M, Value(0, nullptr));

    // Initialize random engine
    distributionA = std::uniform_int_distribution<uint64_t>(1, ~static_cast<uint64_t>(0));
    distributionB = std::uniform_int_distribution<uint64_t>(0, (ONE << wMinusM) - ONE);

    // Initialize random parameters
    randomizeAB();

    // Debug
    #if DEBUG > 0
    allCounter = 0;
    nptrCounter = 0;
    mismatchCounter = 0;
    #endif
}

Universal::~Universal() {
    freeAnalyzers();

    #if DEBUG > 1
    std::cout << "Nullptr hits: " << nptrCounter << "/" << allCounter << std::endl;
    std::cout << "Mismatch hits: " << mismatchCounter << "/" << allCounter << std::endl;
    std::cout << "Correct hits: " << allCounter - mismatchCounter - nptrCounter << "/" << allCounter << std::endl;
    #endif
}

bool Universal::registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) {
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

void Universal::registerAnalyzers(const std::map<identifier_t, analyzer_builder> &analyzer_builders) {
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

IAnalyzer * Universal::lookup(identifier_t identifier) {
    uint64_t hashedID = hash(identifier);

    // The hashedID can't be larger than the number of bins
    assert(hashedID < table.size() && "Hashed ID is outside of the hash table range!");

    Value entry = table[hashedID];

    #if DEBUG > 1
    std::cout << identifier << " -> " << hashedID << std::endl;
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

size_t Universal::size() {
    size_t result = 0;
    for (const auto& current : table) {
        if (current.second != nullptr) {
            result++;
        }
    }
    return result;
}

void Universal::clear() {
    // Free all analyzers
    freeAnalyzers();

    setBins(2);
    table = std::vector<Value>(ONE << M, Value(0, nullptr));
    randomizeAB();
}

size_t Universal::bucketCount() {
    return table.size();
}

void Universal::rehash() {
    // Intermediate representation is just the current table without nulls
    rehash(createIntermediate());
}

// #######################
// ####### PRIVATE #######
// #######################

void Universal::stringifyAnalyzersState(std::ostream &os) const {
    for (const auto &current : table) {
        if (current.second != nullptr) {
            os << *current.second << "\n";
        }
    }
}

void Universal::freeAnalyzers() {
    for (auto& current : table) {
        delete current.second;
        current.second = nullptr;
    }
}

void Universal::rehash(const std::vector<Value>& intermediate) {
    while (!findCollisionFreeHashFunction(intermediate)) {
        #if DEBUG > 0
        std::cout << "Rehashing did not work. Increasing #bins to " << (uint64_t) std::pow(2, M + 1) << " (" << M + 1 << "bit)." << std::endl;
        #endif

        setBins(M + 1);
    }
}

bool Universal::findCollisionFreeHashFunction(const std::vector<Value>& intermediate) {
    // Don't even try if the number of values is larger than the number of buckets
    if (ONE << M < intermediate.size()) {
        return false;
    }

    // Remember the hash function parameters to not break the table if rehashing doesn't work
    uint64_t stored_a = a;
    uint64_t stored_b = b;

    // Because the hash function hashes all values in the universe uniformly to m bins with probability 1/m
    // we should at least try a multiple of #bins times.
    for (size_t i = 1; i <= (ONE << M); i++) {
        // Step 1: Re-randomize hash function parameters
        randomizeAB();

        // Step 2: Create new table
        std::vector<Value> newTable(ONE << M, Value(0, nullptr));

        // Step 3: Try to insert all elements into the new table with the new hash function
        bool finished = true;
        for (const auto &current : intermediate) {
            uint64_t hashedID = hash(current.first);
            assert(hashedID < newTable.size());
            if (newTable[hashedID].second == nullptr) {
                // Free bin, insert the value
                newTable[hashedID] = current;
            } else {
                // The bin is not empty which means there is a collision
                // (there are no duplicates in the intermediate representation so that can't be the case)
                finished = false;
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
            #if DEBUG > 0
            std::cout << "Took " << i << " rehash(es) to resolve." << std::endl;
            #endif
            table = newTable;
            return true;
        }
    }

    // Finding a collision free hash function failed. Revert the hash function parameters.
    a = stored_a;
    b = stored_b;
    return false;
}

size_t Universal::real_size() {
    // every value has 2 byte id + 8 byte analyzer pointer
    return table.size() * 10;
}
