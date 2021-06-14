#pragma once

#include "Defines.h"
#include "dispatchers/IDispatcher.h"

class Array : public IDispatcher {
public:
    Array();
    ~Array() override;

    bool registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) override;
    IAnalyzer *lookup(identifier_t identifier) override;
    size_t size() override;

    size_t real_size() override;

    void clear() override;

private:
    IAnalyzer* table[MAX_IDENTIFIERS]{};
    void stringifyAnalyzersState(std::ostream &os) const override;

    void freeAnalyzers();
};