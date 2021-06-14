#include "dispatchers/TreeMap.h"

TreeMap::~TreeMap() {
    freeAnalyzers();
}

bool TreeMap::registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) {
    return table.emplace(identifier, make_analyzer()).second;
}

IAnalyzer * TreeMap::lookup(identifier_t identifier) {
    if (table.count(identifier) != 0) {
        return table.at(identifier);
    } else {
        return nullptr;
    }
}

size_t TreeMap::size() {
    return table.size();
}

void TreeMap::clear() {
    freeAnalyzers();
    table.clear();
}

void TreeMap::stringifyAnalyzersState(std::ostream &os) const {
    for (const auto &current : table) {
        os << *current.second << "\n";
    }
}

void TreeMap::freeAnalyzers() {
    for (auto &current : table) {
        delete current.second;
        current.second = nullptr;
    }
}

size_t TreeMap::real_size() {
    // Each node uses 2 byte id + 8 byte analyzer pointer, each edge 8 byte pointer
    // A binary tree always has n-1 edges
    return table.size() * 10 + (table.size() - 1) * 8;
}

