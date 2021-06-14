#pragma once

#include "cuckoo_hash.h"

#include "Defines.h"
#include "dispatchers/IDispatcher.h"

class Cuckoo : public IDispatcher {
public:
    Cuckoo();
    ~Cuckoo() override;

    bool registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) override;
    IAnalyzer *lookup(identifier_t identifier) override;
    size_t size() override;
    void clear() override;

    size_t real_size() override;

private:
    cuckoo_hash table;
    std::vector<std::unique_ptr<identifier_t>> keys;
    void stringifyAnalyzersState(std::ostream &os) const override;

    void freeAnalyzers();
};