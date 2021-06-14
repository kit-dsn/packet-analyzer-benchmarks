#include "dispatchers/hashtables/SparseUpper.h"

bool SparseUpper::registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) {
    if (map.empty()) {
        createFragment(identifier, make_analyzer);
        return true;
    }

    // Check if the identifier is after the last fragment. If yes, create a new fragment.
    if (identifier > map.rbegin()->first) {
        createFragment(identifier, make_analyzer);
        // Compress the second-to-last with the last (the new) element.
        compress(std::prev(map.end(), 2), std::prev(map.end()));
        return true;
    }

    // Get correct range for the new identifier (the first that is larger or the same)
    auto ptr = map.lower_bound(identifier);
    identifier_t upperBound = ptr->first;
    table_t &table = ptr->second;
    identifier_t lowerBound = upperBound - table.size() + 1;

    // If the vector has not enough space to fit the new entry, enlarge it or create a new fragment
    if (identifier < lowerBound) {
        // Add the possible inserted nullptrs when resizing to the number of empty slots at the start of the fragment
        // If that gap is too large, generate new fragment
        if (emptiesAtFragmentStart(table) + (lowerBound - identifier - 1) > maxGap) {
           createFragment(identifier, make_analyzer);

           // Compress the distance between the new fragment and the previous. lower_bound gets the inserted fragment.
           // Only compress if the new fragment isn't the first one now (std::prev is invalid in this case).
           if (map.lower_bound(identifier) != map.begin()) {
               compress(std::prev(map.lower_bound(identifier)), map.lower_bound(identifier));
           }
        } else {
            // Gap is small enough, resize and add
            table.resize(upperBound - identifier + 1, nullptr);
            table[upperBound - identifier] = make_analyzer();

            // Merge fragments if the gap between them got too small now
            // Only compress if the current fragment isn't the first one (std::prev is invalid in this case).
            if (ptr != map.begin()) {
                compress(std::prev(ptr), ptr);
            }
        }
        return true;
    } else if (table[upperBound - identifier] != nullptr) {
        // There is already an analyzer registered
        return false;
    } else {
        // There is already a "hole" in a fragment, insert it there
        table[upperBound - identifier] = make_analyzer();

        // Merge fragments if the gap between them got too small now
        compress(std::prev(ptr), ptr);
        return true;
    }
}

IAnalyzer * SparseUpper::lookup(identifier_t identifier) {
    // Get the correct fragment
    auto ptr = map.lower_bound(identifier);
    if (ptr == map.end()) {
        // identifier is larger than the upper bound of the last element.
        // Exiting early to avoid dereferencing the end() iterator.
        return nullptr;
    }
    identifier_t upperBound = ptr->first;
    table_t &table = ptr->second;

    // If the identifier is smaller than the lower bound of the fragment, there is no analyzer registered.
    if (identifier < upperBound - table.size() + 1) {
        return nullptr;
    }

    return table[upperBound - identifier];
}

void SparseUpper::stringifyAnalyzersState(std::ostream &os) const {
#if DEBUG
    int64_t prevUpper = -1;
#endif

    for (const auto &fragment : map) {
#if DEBUG
        if (prevUpper != -1) {
            size_t gapSize = fragment.first - fragment.second.size() - prevUpper;
            assert(gapSize > maxGap && "A gap between fragments can't be smaller than maxGap!");
            os << "----- Gap - " << gapSize << " elements -----" << std::endl;
        }
#endif

        for (const auto &current : fragment.second) {
            if (current != nullptr) {
                os << "[Fragment " << fragment.first << "] " << *current << std::endl;
            }
#if DEBUG
            else {
                os << "[Fragment " << fragment.first << "] EMPTY SLOT" << std::endl;
            }
#endif
        }

#if DEBUG
        prevUpper = fragment.first;
#endif
    }
}

