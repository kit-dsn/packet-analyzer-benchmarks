#pragma once

#include <map>
#include "Defines.h"
#include "dispatchers/hashtables/Sparse.h"

class SparseUpper : public Sparse {
public:
    explicit SparseUpper(uint32_t maxGap = 3) : Sparse(maxGap) {
    }

    bool registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) override;
    IAnalyzer *lookup(identifier_t identifier) override;

private:
    void stringifyAnalyzersState(std::ostream &os) const override;

    static inline size_t emptiesAtFragmentStart(table_t &table) {
        size_t nullCounter = 0;
        for (const auto &current : table) {
            if (current == nullptr) {
                nullCounter++;
            } else {
                break;
            }
        }
        return nullCounter;
    }

    inline void compress(std::map<identifier_t, table_t>::iterator firstFragment, std::map<identifier_t, table_t>::iterator secondFragment) {
        // If the gap between firstFragment->upperBound and secondFragment->lowerBound is too large, don't compress
        size_t interFragmentSpaceSize = (secondFragment->first - secondFragment->second.size()) - firstFragment->first;
        if (interFragmentSpaceSize > maxGap) {
            return;
        }

        // Count consecutive nullptr entries at the start of the second fragment and
        // add the gap between firstFragment->upper and secondFragment->lower
        size_t sum = emptiesAtFragmentStart(secondFragment->second) + interFragmentSpaceSize;

        // If the gap is smaller than the maximum gap, merge the fragments
        if (sum <= maxGap) {
            // Need to swap around the fragments for insertion because elements are reverse ordered in the vector
            // v[0] = 5
            // v[1] = 4
            // v[2] = null (would be 3)
            // That means if firstFragment = 5 4 N and secondFragment is 8 7 N
            // mergeFragments(firstFragment, secondFragment) = 5 4 N 8 7 N
            // instead of the correct result
            // mergeFragments(secondFragment, firstFragment) = 8 7 N 5 4 N
            mergeFragments(secondFragment, firstFragment, interFragmentSpaceSize, secondFragment->first);
        }
    }
};