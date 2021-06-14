#ifndef PROTOTYPE_TIMING_H
#define PROTOTYPE_TIMING_H

#include <string>

namespace Timing {
    std::string fmtTime(uint64_t time);

    void startTM(const std::string &identifier);

    void endTM(const std::string &identifier);

    void printAllTM();

    void printAllTMRelative(const std::string &identifier);

    void printAllTMAvg();

    void printAllTMAvgRelative(const std::string &identifier);

    void printAllTMOPS();

    void printAllTMOPSRelative(const std::string &identifier);

    void toCSV(const std::string &name);
}

#endif //PROTOTYPE_TIMING_H
