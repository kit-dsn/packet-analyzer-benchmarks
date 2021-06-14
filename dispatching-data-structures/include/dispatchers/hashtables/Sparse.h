#pragma once

#include <map>
#include "Defines.h"
#include "dispatchers/IDispatcher.h"

class Sparse : public IDispatcher {
public:
    explicit Sparse(uint32_t maxGap = 3) : maxGap(maxGap) {
    }
    ~Sparse() override;

    bool registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) override;
//    void registerAnalyzers(const std::unordered_map<identifier_t, analyzer_builder> &analyzer_builders) override;
    IAnalyzer *lookup(identifier_t identifier) override;
    size_t size() override;
    void clear() override;

    size_t real_size() override;

protected:
    using table_t = std::vector<IAnalyzer*>;

    uint32_t maxGap;
    // Maps the range as "lowest-identifier" (e.g. range 6-10 is 6) to the vector that contains the range
    std::map<identifier_t, table_t> map;

    inline void createFragment(identifier_t identifier, const analyzer_builder &make_analyzer) {
        // Insert the new element as first element in the fragment.
        table_t newFragment;
        newFragment.push_back(make_analyzer());
        map.emplace(identifier, std::move(newFragment));
    }

    inline void mergeFragments(std::map<identifier_t, table_t>::iterator firstFragment,
            std::map<identifier_t, table_t>::iterator secondFragment,
            identifier_t interFragmentSpaceSize,
            identifier_t idOfNewFragment) {
        table_t newFragment;
        newFragment.reserve(firstFragment->second.size() + secondFragment->second.size());
        // Add all entries from the first fragment
        for (const auto &current : firstFragment->second) {
            newFragment.push_back(current);
        }

        // Add nullptr entries for the interFragmentSpace
        for (size_t i = 0; i < interFragmentSpaceSize; i++) {
            newFragment.push_back(nullptr);
        }

        // Add all entries from the second fragment
        for (const auto &current : secondFragment->second) {
            newFragment.push_back(current);
        }

        // Erase the two fragments that are being merged but remember the lowerbound of the first
        // which is also the lowerbound of the new fragment
        map.erase(firstFragment);
        map.erase(secondFragment);

        // Insert the new fragment
        map.emplace(idOfNewFragment, std::move(newFragment));
    }

private:
    void stringifyAnalyzersState(std::ostream &os) const override;

    void freeAnalyzers();

    static inline size_t emptiesAtFragmentEnd(table_t &table) {
        size_t nullCounter = 0;
        for (size_t i = table.size() - 1; i > 0; i--) {
            if (table[i] == nullptr) {
                nullCounter++;
            } else {
                break;
            }
        }
        return nullCounter;
    }

    inline void compress(std::map<identifier_t, table_t>::iterator firstFragment, std::map<identifier_t, table_t>::iterator secondFragment) {
        // If the gap between firstFragment->upperBound and secondFragment->lowerBound is too large, don't compress
        size_t interFragmentSpaceSize = secondFragment->first - (firstFragment->first + firstFragment->second.size());
        if (interFragmentSpaceSize > maxGap) {
            return;
        }

        // Count consecutive nullptr entries at the end of the first fragment and
        // add the gap between firstFragment->upper and secondFragment->lower
        size_t sum = emptiesAtFragmentEnd(firstFragment->second) + interFragmentSpaceSize;

        // If the gap is smaller than the maximum gap, merge the fragments
        if (sum <= maxGap) {
            mergeFragments(firstFragment, secondFragment, interFragmentSpaceSize, firstFragment->first);
        }
    }
};