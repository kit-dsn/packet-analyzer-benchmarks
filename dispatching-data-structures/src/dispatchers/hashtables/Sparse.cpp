#include "dispatchers/hashtables/Sparse.h"

Sparse::~Sparse() {
    freeAnalyzers();
}

bool Sparse::registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) {
    if (map.empty()) {
        createFragment(identifier, make_analyzer);
        return true;
    }

    // Check if the identifier is before the first fragment. If yes, create a new fragment.
    if (map.begin()->first > identifier) {
        createFragment(identifier, make_analyzer);
        compress(map.begin(), std::next(map.begin()));
        return true;
    }

    // Get correct range for the new identifier (the first that is larger - 1)
    auto ptr = std::prev(map.upper_bound(identifier));
    identifier_t lowerBound = ptr->first;
    table_t &table = ptr->second;
    identifier_t upperBound = lowerBound + table.size() - 1;

    // If the vector has not enough space to fit the new entry, enlarge it or create a new fragment
    if (upperBound < identifier) {
        // Add the possible inserted nullptrs when resizing to the number of empty slots at the end of the fragment
        // If that gap is too large, generate new fragment
        if (emptiesAtFragmentEnd(table) + (identifier - upperBound - 1) > maxGap) {
            createFragment(identifier, make_analyzer);

            // Compress the distance between the new fragment and the next. lower_bound gets the inserted fragment.
            // Only compress if the new fragment isn't the last one now (std::next is invalid, i.e. end(), in this case).
            if (std::next(map.lower_bound(identifier)) != map.end()) {
                compress(map.lower_bound(identifier), std::next(map.lower_bound(identifier)));
            }
        } else {
            // Gap is small enough, resize and add
            table.resize(identifier - lowerBound + 1, nullptr);
            table[identifier - lowerBound] = make_analyzer();

            // Merge fragments if the gap between them got too small now.
            // Only compress if the current fragment isn't the last one (std::next is invalid, i.e. end(), in this case).
            if (std::next(ptr) != map.end()) {
                compress(ptr, std::next(ptr));
            }
        }
        return true;
    } else if (table[identifier - lowerBound] != nullptr) {
        // There is already an analyzer registered
        return false;
    } else {
        // There is already a "hole" in a fragment, insert it there
        table[identifier - lowerBound] = make_analyzer();

        // Merge fragments if the gap between them got too small now
        compress(ptr, std::next(ptr));
        return true;
    }
}

IAnalyzer * Sparse::lookup(identifier_t identifier) {
    // Get the correct fragment
    auto ptr = std::prev(map.upper_bound(identifier));
    identifier_t lowerBound = ptr->first;
    table_t &table = ptr->second;

    // Check if identifier is in bounds of the fragment. If not, there is no analyzer registered.
    if (identifier < lowerBound || identifier >= lowerBound + table.size()) {
        return nullptr;
    }

    return table[identifier - lowerBound];
}

size_t Sparse::size() {
    size_t size = 0;
    for (const auto &fragment : map) {
        for (const auto &current : fragment.second) {
            if (current != nullptr) {
                size++;
            }
        }
    }
    return size;
}

void Sparse::clear() {
    freeAnalyzers();
    map.clear();
}

void Sparse::stringifyAnalyzersState(std::ostream &os) const {
    #if DEBUG
    int64_t prevUpper = -1;
    #endif

    for (const auto &fragment : map) {
        #if DEBUG
        if (prevUpper != -1) {
            size_t gapSize = fragment.first - prevUpper;
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
        prevUpper = fragment.first + fragment.second.size();
        #endif
    }
}

void Sparse::freeAnalyzers() {
    for (auto &fragment : map) {
        for (auto &current : fragment.second) {
            delete current;
            current = nullptr;
        }
    }
}

size_t Sparse::real_size() {
    // Each node uses 2 byte id + 8 byte pointer to the stored vector
    // A binary tree always has n-1 edges, each edge 8 byte pointer
    size_t counter = map.size() * 10 + (map.size() - 1) * 8;
    for (auto &fragment : map) {
        // Each vector contains 8 byte analyzer pointers
        counter += fragment.second.size() * 8;
    }
    return counter;
}
