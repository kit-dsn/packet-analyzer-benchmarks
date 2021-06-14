#include <sstream>
#include <iomanip>

#include "MyPacket.h"

size_t MyPacket::objCount = 0;

size_t MyPacket::getMaxIdentifierSize() {
    return sizeof(identifier_t);
}

const std::vector<identifier_t> &MyPacket::getIdentifiers() const {
    return identifiers;
}

void MyPacket::addIdentifier(identifier_t identifier) {
    identifiers.push_back(identifier);
}

uint32_t MyPacket::getNumber() const {
    return number;
}

void MyPacket::setNumber(uint32_t _number) {
    this->number = _number;
}

const std::vector<uint8_t> &MyPacket::getPayload() const {
    return payload;
}

std::string MyPacket::getPayloadAsByteString() const {
    std::stringstream result;
    result << std::uppercase << std::setfill('0') << std::hex;
    for (uint8_t byte : payload) {
        result << std::setw(2) << (int) byte << " ";
    }
    return result.str();
}

void MyPacket::setPayload(const std::vector<uint8_t> &_payload) {
    MyPacket::payload = _payload;
}

void MyPacket::setPayload(const uint8_t* _payload, size_t payloadLen) {
    payload.reserve(payloadLen);
    for (size_t i = 0; i < payloadLen; i++) {
        payload.push_back(_payload[i]);
    }
}

void MyPacket::setPayload(const std::string &_payload) {
    payload.reserve(_payload.length() / 2); // Two chars are 1 uint8_t
    for (size_t i = 0; i < _payload.length(); i += 2) {
        uint8_t byte = std::stoul(_payload.substr(i, 2), nullptr, 16);
        payload.push_back(byte);
    }
}

std::ostream &operator<<(std::ostream &os, const MyPacket &packet) {
    int digits = 0;
    size_t number = packet.getNumber();
    while (number != 0) {
        number /= 10;
        digits++;
    }

    // For stdout debugging
#if DEBUG > 0
    os << "[Packet " << std::setw(digits) << std::setfill(' ') << packet.getNumber() << "] ";
    for (const auto& current : packet.identifiers) {
        os << "0x" << std::hex << std::setw(2) << std::setfill('0') << current << " " << std::dec;
    }
    os << "<payload>";
#else
    // For writing custom file format
    for (const auto& current : packet.identifiers) {
        os << std::hex << current << " ";
    }
    os << "A" << std::endl;
#endif

    return os;
}