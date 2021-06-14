#include "analyzers/TCPAnalyzer.h"

void TCPAnalyzer::analyze(const MyPacket &packet) {
    counter++;
}

void TCPAnalyzer::print(std::ostream &os) const {
    os << "TCP packets: " << counter;
}
