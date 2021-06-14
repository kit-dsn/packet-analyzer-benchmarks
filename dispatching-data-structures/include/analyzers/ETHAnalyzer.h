#ifndef PROTOTYPE_ETHANALYZER_H
#define PROTOTYPE_ETHANALYZER_H

#include "IAnalyzer.h"

class ETHAnalyzer : public IAnalyzer {
public:
    void analyze(const MyPacket &packet) override;

private:
    size_t counter = 0;

    void print(std::ostream &os) const override;
};


#endif //PROTOTYPE_ETHANALYZER_H
