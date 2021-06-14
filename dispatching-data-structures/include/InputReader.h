#ifndef PROTOTYPE_INPUTREADER_H
#define PROTOTYPE_INPUTREADER_H

#include "Defines.h"
#include "MyPacket.h"
#include "analyzers/IAnalyzer.h"

#include <string>
#include <map>

#define ANALYZER_EMPLACE(identifier, analyzer, target) \
    target.emplace(std::make_pair(identifier, MAKE_ANALYZER_BUILDER(analyzer)));

#define ANALYZER_IF(identifier, input, analyzer, target) \
    if (input == #analyzer) { \
        ANALYZER_EMPLACE(identifier, analyzer, target) \
    }

#define ANALYZER_ELIF(identifier, input, analyzer, target)  \
    else if (input == #analyzer) { \
        ANALYZER_EMPLACE(identifier, analyzer, target) \
    }

#define ANALYZER_ELSE(input, line) \
    else { \
        throw std::invalid_argument("Line " + std::to_string(line) + ": Invalid analyzer " + input + "."); \
    }

enum FileType {
    PCAP,
    CUSTOM_PACKET_FORMAT,
    ANALYZER,
    UNKNOWN
};

class InputReader {
public:
    [[nodiscard]] static std::vector<MyPacket> readPacketFile(const std::string &path);
    [[nodiscard]] static std::map<identifier_t, analyzer_builder> readAnalyzerFile(const std::string &path);

private:
    static FileType getFileType(const std::string &path);
    template <class T>
    static identifier_t extractWiFiTypeSubtypeIdentifier(T* pdu);
    [[nodiscard]] static std::vector<MyPacket> readPCAP(const std::string &path, const std::string &writeToFile="");
    [[nodiscard]] static std::vector<MyPacket> readCustomFileFormat(const std::string &path);
    [[nodiscard]] static identifier_t parseIdentifier(const std::string &field, size_t currentLineNum);
};


#endif //PROTOTYPE_INPUTREADER_H
