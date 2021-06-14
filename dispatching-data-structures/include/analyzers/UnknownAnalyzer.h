#ifndef PROTOTYPE_UNKNOWNANALYZER_H
#define PROTOTYPE_UNKNOWNANALYZER_H

#include "IAnalyzer.h"

class UnknownAnalyzer : public IAnalyzer {
public:
    void analyze(const MyPacket &packet) override;

private:
    size_t counter = 0;

    void print(std::ostream &os) const override;
};


#endif //PROTOTYPE_UNKNOWNANALYZER_H
