#include "analyzers/UnknownAnalyzer.h"

void UnknownAnalyzer::analyze(const MyPacket &packet) {
    counter++;
}

void UnknownAnalyzer::print(std::ostream &os) const {
    os << "Unknown packets: " << counter;
}
