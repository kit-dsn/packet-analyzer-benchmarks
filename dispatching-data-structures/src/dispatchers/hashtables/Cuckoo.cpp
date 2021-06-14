#include "dispatchers/hashtables/Cuckoo.h"
#include "cuckoo_hash.h"

Cuckoo::Cuckoo() {
    cuckoo_hash_init(&table, 1);
}

Cuckoo::~Cuckoo() {
    freeAnalyzers();
    cuckoo_hash_destroy(&table);
}

bool Cuckoo::registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) {
    keys.push_back(std::make_unique<identifier_t>(identifier));
    return cuckoo_hash_insert(&table, keys[keys.size() - 1].get(), sizeof(identifier), make_analyzer()) == nullptr;
}

IAnalyzer * Cuckoo::lookup(identifier_t identifier) {
    cuckoo_hash_item* result = cuckoo_hash_lookup(&table, &identifier, sizeof(identifier));
    if (result != nullptr) {
        return static_cast<IAnalyzer*>(result->value);
    } else {
        return nullptr;
    }
}

size_t Cuckoo::size() {
    return cuckoo_hash_count(&table);
}


void Cuckoo::clear() {
    freeAnalyzers();
}


void Cuckoo::stringifyAnalyzersState(std::ostream &os) const {
    cuckoo_hash_item* current = nullptr;
    for (cuckoo_hash_each(current, &table)) {
        os << *static_cast<IAnalyzer*>(current->value) << "\n";
    }
}

void Cuckoo::freeAnalyzers() {
    cuckoo_hash_item* current = nullptr;
    for (cuckoo_hash_each(current, &table)) {
        delete static_cast<IAnalyzer*>(current->value);
        cuckoo_hash_remove(&table, current);
    }
}

size_t Cuckoo::real_size() {
    return (1u << table.power) * table.bin_size * (sizeof(cuckoo_hash_item) + 8);
}
