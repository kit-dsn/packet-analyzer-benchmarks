#ifndef PROTOTYPE_TCPANALYZER_H
#define PROTOTYPE_TCPANALYZER_H

#include "IAnalyzer.h"

class TCPAnalyzer : public IAnalyzer {
public:
    void analyze(const MyPacket &packet) override;

private:
    size_t counter = 0;

    void print(std::ostream &os) const override;
};


#endif //PROTOTYPE_TCPANALYZER_H
