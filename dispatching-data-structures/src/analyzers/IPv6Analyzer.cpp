#include "analyzers/IPv6Analyzer.h"

void IPv6Analyzer::analyze(const MyPacket &packet) {
    counter++;
}

void IPv6Analyzer::print(std::ostream &os) const {
    os << "IPv6 packets: " << counter;
}
