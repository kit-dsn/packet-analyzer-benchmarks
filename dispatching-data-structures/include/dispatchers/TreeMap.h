#ifndef PROTOTYPE_TREEMAP_H
#define PROTOTYPE_TREEMAP_H

#include <map>
#include "Defines.h"
#include "dispatchers/IDispatcher.h"

class TreeMap : public IDispatcher {
public:
    ~TreeMap() override;

    bool registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) override;
    IAnalyzer *lookup(identifier_t identifier) override;
    size_t size() override;
    void clear() override;

    size_t real_size() override;

private:
    std::map<identifier_t, IAnalyzer*> table;
    void stringifyAnalyzersState(std::ostream &os) const override;

    void freeAnalyzers();
};

#endif //PROTOTYPE_TREEMAP_H
