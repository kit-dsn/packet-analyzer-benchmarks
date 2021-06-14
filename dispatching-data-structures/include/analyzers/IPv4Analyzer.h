#ifndef PROTOTYPE_IPV4ANALYZER_H
#define PROTOTYPE_IPV4ANALYZER_H

#include "IAnalyzer.h"

class IPv4Analyzer : public IAnalyzer {
public:
    void analyze(const MyPacket &packet) override;

private:
    size_t counter = 0;

    void print(std::ostream &os) const override;
};


#endif //PROTOTYPE_IPV4ANALYZER_H
