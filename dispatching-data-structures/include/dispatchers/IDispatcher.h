#ifndef PROTOTYPE_IDISPATCHER_H
#define PROTOTYPE_IDISPATCHER_H

#include <map>

#include "Defines.h"
#include "analyzers/IAnalyzer.h"

class IDispatcher {
public:
    virtual ~IDispatcher() = default;
    virtual bool registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) = 0;
    virtual void registerAnalyzers(const std::map<identifier_t, analyzer_builder> &analyzer_builders) {
        for (auto &current : analyzer_builders) {
            if (!registerAnalyzer(current.first, current.second)) {
                throw std::invalid_argument("Analyzer " + std::to_string(current.first) + " already registered!");
            }
        }
    }
    virtual IAnalyzer * lookup(identifier_t identifier) = 0;

    /**
     * This function reports how many analyzers are currently registered in the dispatcher.
     *
     * @return The amount of registered analyzers
     */
    virtual size_t size() = 0;
    virtual size_t real_size() = 0;

    virtual void clear() = 0;

    friend std::ostream &operator<<(std::ostream &os, const IDispatcher &dispatcher) {
        dispatcher.stringifyAnalyzersState(os);
        return os;
    };

private:
    virtual void stringifyAnalyzersState(std::ostream& os) const = 0;
};

#endif //PROTOTYPE_IDISPATCHER_H
