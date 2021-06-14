#ifndef PROTOTYPE_HANOV_H
#define PROTOTYPE_HANOV_H

#include <iomanip>
#include <bitset>

#include "Defines.h"
#include "dispatchers/IDispatcher.h"

#define HASH_CONST 0x01000193

class Hanov : public IDispatcher {
public:
    Hanov() : empty(true), first_d(0), _size(0) {}
    ~Hanov() override;
    bool registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) override;
    void registerAnalyzers(const std::map<identifier_t, analyzer_builder> &analyzer_builders) override;
    IAnalyzer *lookup(identifier_t identifier) override;
    size_t size() override;
    void clear() override;

    size_t real_size() override;

private:
    bool empty;
    uint32_t first_d;
    std::vector<uint32_t> intermediate;
    std::vector<Value> values;
    size_t _size;

    void createMPH(std::unordered_map<identifier_t, IAnalyzer*> &&analyzers);
    void stringifyAnalyzersState(std::ostream &os) const override;

    void freeAnalyzers();

    inline static bool checkMultiple(uint64_t multiplier, size_t numAnalyzers) {
        return (numAnalyzers % multiplier == 0   // Divisible without rest by multiplier
                && ((numAnalyzers / multiplier) & ((numAnalyzers / multiplier) - 1)) == 0); // Is it actually 2^n
    }

    inline static uint32_t hash(uint64_t d, uint64_t input) {
        if (d == 0) {
            d = HASH_CONST;
        }

        return ((d * HASH_CONST) ^ input) & 0xFFFFFFFF;
    }

    #if DEBUG
    inline static void hashDebug(uint64_t d, uint64_t input, uint64_t mod) {
        if (d == 0) {
            d = HASH_CONST;
        }

        std::cout << "d=" << std::setw(9) << std::setfill('0') << d
                  << ", input=" << std::setw(9) << std::setfill(' ') << input
                  << ", XOR_LEFT=" << std::bitset<64>(d * HASH_CONST)
                  << ", XOR=" << std::bitset<64>((d * HASH_CONST) ^ input)
                  << ", filtered=" << std::bitset<32>(hash(d, input))
                  << ", mod=" << std::bitset<32>(hash(d, input) % mod)
                  << ", num=" << mod
                  << std::endl;
    }
    #endif
};

#endif //PROTOTYPE_HANOV_H
