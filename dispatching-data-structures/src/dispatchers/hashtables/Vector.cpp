#include "dispatchers/hashtables/Vector.h"

Vector::~Vector() {
    freeAnalyzers();
}

bool Vector::registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) {
    // If the table has size 1 and the entry is nullptr, there was nothing added yet. Just add it.
    if (table.size() == 1 && table[0] == nullptr) {
        table[0] = make_analyzer();
        lowestIdentifier = identifier;
        return true;
    }

    // If highestIdentifier == identifier, overwrite would happen -> no check needed, will return false
    if (getHighestIdentifier() < identifier) {
        table.resize(table.size() + (identifier - getHighestIdentifier()), nullptr);
    } else if (identifier < lowestIdentifier) {
        // Lower than the lowest registered identifier. Shift up by lowerBound - identifier
        identifier_t distance = lowestIdentifier - identifier;
        table.resize(table.size() + distance, nullptr);

        // Shift values
        for (ssize_t i = table.size() - 1; i >= 0; i--) {
            if (table[i] != nullptr) {
                table.at(i + distance) = table.at(i);
                table.at(i) = nullptr;
            }
        }

        lowestIdentifier = identifier;
    }

    int64_t index = identifier - lowestIdentifier;
    if (table[index] == nullptr) {
        table[index] = make_analyzer();
        return true;
    }
    return false;
}

void Vector::registerAnalyzers(const std::map<identifier_t, analyzer_builder> &analyzer_builders) {
    // Search smallest and largest identifier and resize vector
    const auto& lowestNew =
            std::min_element(analyzer_builders.begin(), analyzer_builders.end(),
                           [](const std::pair<identifier_t , analyzer_builder> &a, const std::pair<identifier_t , analyzer_builder> &b) {
                               return a.first < b.first;
                           });

    // Register lowest first in order to do shifting only once
    registerAnalyzer(lowestNew->first, lowestNew->second);
    for (auto i = analyzer_builders.begin(); i != analyzer_builders.end(); i++) {
        // Already added if i == lowestNew
        if (i == lowestNew) {
            continue;
        }

        if (!registerAnalyzer(i->first, i->second)) {
            throw std::invalid_argument("Analyzer already registered!");
        }
    }
}

IAnalyzer * Vector::lookup(identifier_t identifier) {
    int64_t index = identifier - lowestIdentifier;
    if (index >= 0 && index < table.size() && table[index] != nullptr) {
        return table[index];
    } else {
        return nullptr;
    }
}

size_t Vector::size() {
    size_t result = 0;
    for (const auto& current : table) {
        if (current != nullptr) {
            result++;
        }
    }
    return result;
}

void Vector::clear() {
    freeAnalyzers();
    table.clear();
}

void Vector::stringifyAnalyzersState(std::ostream &os) const {
    for (const auto &current : table) {
        if (current != nullptr) {
            os << *current << "\n";
        }
    }
}

void Vector::freeAnalyzers() {
    for (auto &current : table) {
        delete current;
        current = nullptr;
    }
}

size_t Vector::real_size() {
    return table.size() * 8;
}

