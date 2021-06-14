#ifndef PROTOTYPE_IANALYZER_H
#define PROTOTYPE_IANALYZER_H

#include <ostream>
#include <memory>
#include <functional>
#include "MyPacket.h"

#define MAKE_ANALYZER_BUILDER(analyzer) []() { return new analyzer; }

class IAnalyzer {
public:
    virtual ~IAnalyzer() = default;
    virtual void analyze(const MyPacket &packet) = 0;

    friend std::ostream &operator<<(std::ostream &os, const IAnalyzer &analyzer) {
        analyzer.print(os);
        return os;
    }

private:
    virtual void print(std::ostream& os) const = 0;
};

using analyzer_builder = std::function<IAnalyzer*()>;

#endif //PROTOTYPE_IANALYZER_H
