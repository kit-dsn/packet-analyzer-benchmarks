#include "analyzers/IPv4Analyzer.h"

void IPv4Analyzer::analyze(const MyPacket &packet) {
    counter++;
}

void IPv4Analyzer::print(std::ostream &os) const {
    os << "IPv4 packets: " << counter;
}
