#pragma once

#include <random>
#include <bitset>

#include "Defines.h"
#include "dispatchers/IDispatcher.h"

class UniversalSim : public IDispatcher {
public:
    UniversalSim();
    ~UniversalSim() override;

    bool registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) override;
    void registerAnalyzers(const std::map<identifier_t, analyzer_builder> &analyzer_builders) override;
    IAnalyzer *lookup(identifier_t identifier) override;
    size_t size() override;
    void clear() override;

    size_t real_size() override;

    size_t bucketCount();

    // Rehashes the hash table including re-randomization of the hash function.
    void rehash();

    /**
     * Tries to find a collision free hash function with the current number of buckets.
     *
     * @param intermediate The key-value set to store in the hashtable.
     * @return true, iff it found a collision-free hash function.
     */
    bool findCollisionFreeHashFunction(const std::vector<Value>& intermediate);

    uint64_t worked;
    uint64_t didntWork;

    inline void setBins(uint8_t newM) {
        if (newM > (sizeof(word_t) * 8)) {
            throw std::runtime_error("Number of bits for bin count too large.");
        }

        M = newM;
        wMinusM = sizeof(word_t) * 8 - M;
        distributionB = std::uniform_int_distribution<uint64_t>(0, (ONE << wMinusM) - ONE);
    }

private:
    // The datatype from which w and the random value size is derived
    using word_t = uint64_t;

    static const uint64_t ONE = 1u;

    // Chosen random constants for the currently selected collision free random hash function
    uint64_t a; // Needs to be a random odd positive value < 2^(sizeof(word_t) * 8)
    uint64_t b; // Needs to be a random non-negative value < 2^(((sizeof(word_t) * 8) - M)

    // Current bits that define the number of bins. Initially 2 which means there are 2^2 = 4 bins.
    uint8_t M = 2;

    // Current shift value which is the number of bits that are "insignificant" because of the universe size.
    uint8_t wMinusM;

    // RNG
    std::random_device rd;
    std::mt19937_64 generator;
    std::uniform_int_distribution<uint64_t> distributionA;
    std::uniform_int_distribution<uint64_t> distributionB;

    // Debug
    #if DEBUG > 0
    size_t nptrCounter;
    size_t mismatchCounter;
    size_t allCounter;
    #endif

    std::vector<Value> table;
    void stringifyAnalyzersState(std::ostream &os) const override;

    void freeAnalyzers();

    void rehash(const std::vector<Value>& intermediate);

    // findCollisionFreeHashFunction belongs here

    [[nodiscard]] inline uint64_t hash(const word_t  value) const {
//        #if DEBUG > 0
//        std::cout << "(" << a << " * " << value << " + " << b << ") >> " << wMinusM << std::endl;
//        std::cout << "\t= (" << std::bitset<32>(a) << " * " << std::bitset<32>(value) << " + " << std::bitset<32>(b) << ") >> " << static_cast<uint64_t>(wMinusM) << std::endl;
//        std::cout << "\t= (" << std::bitset<64>(a * value) << " + " << std::bitset<64>(b) << ") >> " << static_cast<uint64_t>(wMinusM) << std::endl;
//        std::cout << "\t=  " << std::bitset<64>(a * value + b) << " >> " << static_cast<uint64_t>(wMinusM) << std::endl;
//        std::cout << "\t=  " << std::bitset<64>(static_cast<word_t>(a * value + b) >> wMinusM) << " (" << (static_cast<word_t>(a * value + b) >> wMinusM) << ")" << std::endl;
//        #endif
        // The cast is the "mod 2^w" and the rshift is the "mod 2^(w-M)"
        return static_cast<word_t>(a * value + b) >> wMinusM;
    }

    inline void randomizeAB() {
        do {
            a = distributionA(generator);
        } while (a % 2 == 0);

        b = distributionB(generator);
    }

    // setBins belongs here

    inline std::vector<Value> createIntermediate() {
        std::vector<Value> intermediate;
        for (const auto &current : table) {
            if (current.second != nullptr) {
                intermediate.emplace_back(current.first, current.second);
            }
        }
        return intermediate;
    }
};