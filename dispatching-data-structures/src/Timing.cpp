#include "Timing.h"

#include <map>
#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <fstream>
#include <numeric>
#include <vector>
#include <algorithm>

std::map<std::string, uint64_t> measurements;
std::map<std::string, std::vector<uint64_t>> allmeasurements;
std::map<std::string, uint64_t> numMeasurements;
std::map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock>> measureStarts;

std::string Timing::fmtTime(uint64_t time) {
    std::stringstream ss;
    ss << std::setprecision(3) << std::fixed << std::setfill(' ') << std::setw(11);
    if (time < 10'000) {
        ss << (double) time << "ns";
    } else if (time < 10'000'000) {
        ss << time / 1E3 << "µs";
    } else if (time < 10'000'000'000){
        ss << time / 1E6 << "ms";
    } else {
        ss << time / 1E9 << "s";
    }

    return ss.str();
}

void Timing::startTM(const std::string &identifier) {
    if (measureStarts.count(identifier) != 0) {
        std::cout << "The timer '" << identifier << "' is already running!" << std::endl;
        abort();
    }

    measureStarts.emplace(identifier, std::chrono::high_resolution_clock::now());
}

void Timing::endTM(const std::string &identifier) {
    auto end = std::chrono::high_resolution_clock::now();
    if(measureStarts.count(identifier) == 0) {
        std::cout << "The timer '" << identifier << "' isn't running!" << std::endl;
        abort();
    }

    long time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - measureStarts.at(identifier)).count();
    if (!measurements.count(identifier)) {
        measurements.emplace(identifier, time);
        numMeasurements.emplace(identifier, 1);
        allmeasurements.emplace(identifier, std::vector<uint64_t>());
        allmeasurements.at(identifier).push_back(time);
    } else {
        measurements.at(identifier) += time;
        numMeasurements.at(identifier)++;
        allmeasurements.at(identifier).push_back(time);
    }

    measureStarts.erase(identifier);
}

size_t getPadding() {
    size_t max = 0;
    for (const auto &current : measurements) {
        max = std::max(current.first.length(), max);
    }

    return max;
}

void Timing::printAllTM() {
    if (!measureStarts.empty()) {
        std::cout << "There is still a timer running: " << measureStarts.begin()->first << std::endl;
        abort();
    }

    size_t padding = getPadding();

    std::cout << std::endl << "##### TIMINGS #####" << std::endl;

    std::cout << std::setprecision(2) << std::fixed;
    for (const auto &entry : measurements) {
        int64_t newTime = entry.second;
        std::cout << std::left << std::setfill(' ') << std::setw(padding) << entry.first << std::right;
        std::cout <<  " - ";
        std::cout << fmtTime(newTime);
        std::cout << std::endl;
    }
}

void Timing::printAllTMRelative(const std::string &identifier) {
    if (!measureStarts.empty()) {
        std::cout << "There is still a timer running: " << measureStarts.begin()->first << std::endl;
        abort();
    }

    if (measurements.count(identifier) == 0) {
        std::cout << "The base timer doesn't exist!" << std::endl;
        abort();
    }

    size_t padding = getPadding();

    std::cout << std::endl << "##### TIMINGS #####" << std::endl;
    int64_t baseTime = measurements[identifier];
    std::cout << std::left << std::setfill(' ') << std::setw(padding) << identifier << " - " << std::right;
    std::cout << fmtTime(baseTime) << std::endl;

    std::cout << std::setprecision(2) << std::fixed;
    for (const auto &entry : measurements) {
        if (entry.first == identifier) {
            continue;
        }

        int64_t newTime = entry.second;
        std::cout << std::left << std::setfill(' ') << std::setw(padding) << entry.first << std::right;
        std::cout <<  " - ";
        std::cout << fmtTime(newTime);
        std::cout << ", ";
        std::cout << std::setfill(' ') << std::setw(7) << (double) (newTime - baseTime) / baseTime * 100;
        std::cout << "%";
        std::cout << ", factor " << std::setfill(' ') << std::setw(5) << (double) newTime / baseTime << std::endl;
    }
}

void Timing::printAllTMAvg() {
    if (!measureStarts.empty()) {
        std::cout << "There is still a timer running: " << measureStarts.begin()->first << std::endl;
        abort();
    }

    if (numMeasurements.size() != measurements.size()) {
        std::cout << "The number of measurement groups is inconsistent!" << std::endl;
        abort();
    }

    size_t padding = getPadding();

    std::cout << std::endl << "##### TIME PER OPERATION #####" << std::endl;
    std::cout << std::setprecision(2) << std::fixed;
    for (const auto &current : measurements) {
        uint64_t count = numMeasurements.at(current.first);
        int64_t newAvg = current.second / count;
        std::cout << std::left << std::setfill(' ') << std::setw(padding) << current.first << std::right;
        std::cout <<  " - ";
        std::cout << fmtTime(newAvg);
        std::cout << std::endl;
    }
}

void Timing::printAllTMAvgRelative(const std::string &identifier) {
    if (!measureStarts.empty()) {
        std::cout << "There is still a timer running: " << measureStarts.begin()->first << std::endl;
        abort();
    }

    if (numMeasurements.size() != measurements.size()) {
        std::cout << "The number of measurement groups is inconsistent!" << std::endl;
        abort();
    }

    if (measurements.count(identifier) == 0) {
        std::cout << "The base timer doesn't exist!" << std::endl;
        abort();
    }

    size_t padding = getPadding();

    std::cout << std::endl << "##### TIME PER OPERATION #####" << std::endl;
    int64_t baseAvg = measurements[identifier] / numMeasurements[identifier];
    std::cout << std::left << std::setfill(' ') << std::setw(padding) << identifier << " - " << std::right;
    std::cout << fmtTime(baseAvg) << std::endl;

    std::cout << std::setprecision(2) << std::fixed;
    for (const auto &current : measurements) {
        if (current.first == identifier) {
            continue;
        }

        uint64_t count = numMeasurements.at(current.first);
        int64_t newAvg = current.second / count;
        std::cout << std::left << std::setfill(' ') << std::setw(padding) << current.first << std::right;
        std::cout <<  " - ";
        std::cout << fmtTime(newAvg);
        std::cout << ", ";
        std::cout << std::setfill(' ') << std::setw(7) << (double) (newAvg - baseAvg) / baseAvg * 100;
        std::cout << "%";
        std::cout << ", factor " << std::setfill(' ') << std::setw(5) << (double) newAvg / baseAvg << std::endl;
    }
}

void Timing::printAllTMOPS() {
    if (!measureStarts.empty()) {
        std::cout << "There is still a timer running: " << measureStarts.begin()->first << std::endl;
        abort();
    }

    if (numMeasurements.size() != measurements.size()) {
        std::cout << "The number of measurement groups is inconsistent!" << std::endl;
        abort();
    }

    size_t padding = getPadding();

    std::cout << std::endl << "##### OPERATIONS PER SECOND #####" << std::endl;
    std::cout << std::setprecision(2) << std::fixed;
    for (const auto &current : measurements) {
        double timeInSecs = current.second / 1E9;
        int64_t newOPS = numMeasurements.at(current.first) / timeInSecs;
        std::cout << std::left << std::setfill(' ') << std::setw(padding) << current.first << std::right;
        std::cout <<  " - ";
        std::cout.imbue(std::locale(""));
        std::cout << std::setfill(' ') << std::setw(13) << newOPS;
        std::cout << " Operations/s";
        std::cout << std::endl;
    }
}

void Timing::printAllTMOPSRelative(const std::string &identifier) {
    if (!measureStarts.empty()) {
        std::cout << "There is still a timer running: " << measureStarts.begin()->first << std::endl;
        abort();
    }

    if (numMeasurements.size() != measurements.size()) {
        std::cout << "The number of measurement groups is inconsistent!" << std::endl;
        abort();
    }

    if (measurements.count(identifier) == 0) {
        std::cout << "The base timer doesn't exist!" << std::endl;
        abort();
    }

    size_t padding = getPadding();

    std::cout << std::endl << "##### OPERATIONS PER SECOND #####" << std::endl;
    int64_t baseOPS = numMeasurements[identifier] / (measurements[identifier] / 1E9);
    std::cout << std::left << std::setfill(' ') << std::setw(padding) << identifier << " - " << std::right;
    std::cout.imbue(std::locale(""));
    std::cout << std::setfill(' ') << std::setw(13) << baseOPS;
    std::cout << " Operations/s" << std::endl;
    std::cout.imbue(std::locale("en_US.UTF8"));

    std::cout << std::setprecision(2) << std::fixed;
    for (const auto &current : measurements) {
        if (current.first == identifier) {
            continue;
        }

        double timeInSecs = current.second / 1E9;
        int64_t newOPS = numMeasurements.at(current.first) / timeInSecs;
        std::cout << std::left << std::setfill(' ') << std::setw(padding) << current.first << std::right;
        std::cout <<  " - ";
        std::cout.imbue(std::locale(""));
        std::cout << std::setfill(' ') << std::setw(13) << newOPS;
        std::cout << " Operations/s";
        std::cout.imbue(std::locale("en_US.UTF8"));
        std::cout << ", ";
        std::cout << std::setfill(' ') << std::setw(7) << (double) (newOPS - baseOPS) / baseOPS * 100;
        std::cout << "%";
        std::cout << ", factor " << std::setfill(' ') << std::setw(5) << (double) newOPS / baseOPS << std::endl;
    }
}

void Timing::toCSV(const std::string &name) {
    if (!measureStarts.empty()) {
        std::cout << "There is still a timer running: " << measureStarts.begin()->first << std::endl;
        abort();
    }

    if (numMeasurements.size() != measurements.size()) {
        std::cout << "The number of measurement groups is inconsistent!" << std::endl;
        abort();
    }

    // Open csv file
    std::ofstream csv;
    std::stringstream ss;
    ss << "benchmark_" << name << ".csv";
    csv.open(ss.str());

    csv << "name,runtime,time/op,stddev,op/s" << std::endl;
    for (const auto &current : measurements) {
        auto v = allmeasurements.at(current.first);
        allmeasurements.erase(allmeasurements.begin());
        double sum = std::accumulate(v.begin(), v.end(), 0.0);
        double mean = sum / v.size();

        std::vector<double> diff(v.size());
        std::transform(v.begin(), v.end(), diff.begin(), [mean](double x) { return x - mean; });
        double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
        double stdev = std::sqrt(sq_sum / v.size());

        csv << std::fixed << current.first << "," << current.second << "," << mean << "," << stdev
            << "," << static_cast<int64_t>(numMeasurements.at(current.first) / (current.second / 1E9)) << std::endl;
    }

    csv.close();
}