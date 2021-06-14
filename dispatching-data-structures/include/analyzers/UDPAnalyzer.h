#ifndef PROTOTYPE_UDPANALYZER_H
#define PROTOTYPE_UDPANALYZER_H

#include "IAnalyzer.h"

class UDPAnalyzer : public IAnalyzer {
public:
    void analyze(const MyPacket &packet) override;

private:
    size_t counter = 0;

    void print(std::ostream &os) const override;
};


#endif //PROTOTYPE_UDPANALYZER_H
