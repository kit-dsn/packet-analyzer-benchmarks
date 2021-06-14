#include "dispatchers/hashtables/Array.h"

Array::Array() {
    for (auto& current : table) {
        current = nullptr;
    }
}

Array::~Array() {
    freeAnalyzers();
}

bool Array::registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) {
    if (table[identifier] == nullptr) {
        table[identifier] = make_analyzer();
        return true;
    }
    return false;
}

IAnalyzer * Array::lookup(identifier_t identifier) {
    if (table[identifier] != nullptr) {
        return table[identifier];
    }
    return nullptr;
}

size_t Array::size() {
    size_t result = 0;
    for (const auto& current : table) {
        if (current != nullptr) {
            result++;
        }
    }
    return result;
}

void Array::clear() {
    freeAnalyzers();
}

void Array::stringifyAnalyzersState(std::ostream &os) const {
    size_t counter = 0;
    for (const auto &current : table) {
        if (current != nullptr) {
            os << "[0x" << std::hex << counter << std::dec << "] " << *current << "\n";
        }
        counter++;
    }
}

void Array::freeAnalyzers() {
    for (auto &current : table) {
        delete current;
        current = nullptr;
    }
}

size_t Array::real_size() {
    return MAX_IDENTIFIERS * 8;
}
