#include "analyzers/UDPAnalyzer.h"

void UDPAnalyzer::analyze(const MyPacket &packet) {
    counter++;
}

void UDPAnalyzer::print(std::ostream &os) const {
    os << "UDP packets: " << counter;
}
