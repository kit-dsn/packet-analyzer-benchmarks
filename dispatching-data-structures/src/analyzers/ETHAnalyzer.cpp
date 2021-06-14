#include "analyzers/ETHAnalyzer.h"

void ETHAnalyzer::analyze(const MyPacket &packet) {
    counter++;
}

void ETHAnalyzer::print(std::ostream &os) const {
    os << "ETH packets: " << counter;
}
