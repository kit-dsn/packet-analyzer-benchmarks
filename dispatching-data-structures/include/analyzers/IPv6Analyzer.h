#ifndef PROTOTYPE_IPv6ANALYZER_H
#define PROTOTYPE_IPv6ANALYZER_H

#include "IAnalyzer.h"

class IPv6Analyzer : public IAnalyzer {
public:
    void analyze(const MyPacket &packet) override;

private:
    size_t counter = 0;

    void print(std::ostream &os) const override;
};


#endif //PROTOTYPE_IPv6ANALYZER_H
