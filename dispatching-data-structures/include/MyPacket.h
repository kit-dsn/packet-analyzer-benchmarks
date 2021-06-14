#ifndef PROTOTYPE_MYPACKET_H
#define PROTOTYPE_MYPACKET_H

#include <cstdint>
#include <utility>
#include <vector>
#include <string>
#include <ostream>

// Should not be something other than uint8_t, uint16_t or uint32_t. Everything else might break stuff.
using identifier_t = uint16_t;

class MyPacket {
public:
    MyPacket() : number(++objCount) {
    }

    explicit MyPacket(std::vector<uint8_t> payload) : number(++objCount), payload(std::move(payload)) {
    }

    [[nodiscard]] static size_t getMaxIdentifierSize();

    [[nodiscard]] uint32_t getNumber() const;
    void setNumber(uint32_t _number);

    [[nodiscard]] const std::vector<identifier_t>& getIdentifiers() const;
    void addIdentifier(identifier_t identifier);

    [[nodiscard]] const std::vector<uint8_t>& getPayload() const;
    [[nodiscard]] std::string getPayloadAsByteString() const;
    void setPayload(const std::vector<uint8_t>& _payload);
    void setPayload(const uint8_t* _payload, size_t payloadLen);
    void setPayload(const std::string& _payload);

    friend std::ostream& operator<<(std::ostream &os, const MyPacket &packet);

private:
    static size_t objCount;
    size_t number;
    std::vector<identifier_t> identifiers;
    std::vector<uint8_t> payload;
};


#endif //PROTOTYPE_MYPACKET_H
