#pragma once

#include "Defines.h"
#include "dispatchers/IDispatcher.h"

class Vector : public IDispatcher {
public:
    Vector() : lowestIdentifier(0), table(std::vector<IAnalyzer*>(1, nullptr)) {}
    ~Vector() override;

    bool registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) override;
    void registerAnalyzers(const std::map<identifier_t, analyzer_builder> &analyzer_builders) override;
    IAnalyzer *lookup(identifier_t identifier) override;
    size_t size() override;
    size_t real_size() override;
    void clear() override;

private:
    identifier_t lowestIdentifier;
    std::vector<IAnalyzer*> table;
    void stringifyAnalyzersState(std::ostream &os) const override;

    void freeAnalyzers();

    inline identifier_t getHighestIdentifier() const {
        return lowestIdentifier + table.size() - 1;
    }
};