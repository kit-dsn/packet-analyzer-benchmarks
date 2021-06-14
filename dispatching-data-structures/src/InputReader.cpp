#include <fstream>
#include <cstring>

#include <iterator>
#include <sstream>

#include "Defines.h"
#include "analyzers/All.h"
#include "MyPacket.h"
#include "InputReader.h"

// ********************
// ****** PUBLIC ******
// ********************
std::vector<MyPacket> InputReader::readPacketFile(const std::string &path) {
    // Determine file type and continue accordingly
    switch(getFileType(path)) {
        case PCAP: {
			throw std::invalid_argument("Not a valid packet file type.");
//            return readPCAP(path);
        }
        case CUSTOM_PACKET_FORMAT: {
            return readCustomFileFormat(path);
        }
        default: {
            throw std::invalid_argument("Not a valid packet file type.");
        }
    }
}

std::map<identifier_t, analyzer_builder> InputReader::readAnalyzerFile(const std::string &path) {
    if (getFileType(path) != ANALYZER) {
        throw std::invalid_argument("Not a valid analyzer file type (Needs to begin with '# ANALYZERS').");
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::invalid_argument("File path does not exist.");
    }

    std::string line;
    size_t lineCounter = 0;
    std::map<identifier_t, analyzer_builder> analyzerBuilders;
    while (std::getline(file, line)) {
        lineCounter++;
        if (line.empty()) {
            throw std::invalid_argument("Line " + std::to_string(lineCounter) + " is empty.");
        }

        // Skip first line with the marker
        if (lineCounter == 1) {
            continue;
        }

        std::istringstream iss(line);
        std::vector<std::string> fields(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());
        if (fields.size() != 2) {
            throw std::invalid_argument("Line " + std::to_string(lineCounter) + ": Wrong syntax, expected <identifier> <analyzer_name>.");
        }

        identifier_t identifier = parseIdentifier(fields[0], lineCounter);
        std::string input = fields[1];
        // Strip leading 0x if it exists
        if (input.substr(0, 2) == "0x") {
            input.replace(0, 2, "");
        }

        ANALYZER_IF(identifier, input, ETHAnalyzer, analyzerBuilders)
        ANALYZER_ELIF(identifier, input, IPv4Analyzer, analyzerBuilders)
        ANALYZER_ELIF(identifier, input, IPv6Analyzer, analyzerBuilders)
        ANALYZER_ELIF(identifier, input, TCPAnalyzer, analyzerBuilders)
        ANALYZER_ELIF(identifier, input, UDPAnalyzer, analyzerBuilders)
        ANALYZER_ELIF(identifier, input, UnknownAnalyzer, analyzerBuilders)
        ANALYZER_ELSE(input, lineCounter)
    }

    return analyzerBuilders;
}


// ********************
// ***** PRIVATE ******
// ********************
FileType InputReader::getFileType(const std::string &path) {
    std::ifstream file(path, std::ifstream::in | std::ifstream::binary);
    if (!file.is_open()) {
        throw std::invalid_argument("File does not exist.");
    }

    file.seekg(0);

    // Read first 4 bytes to uint32_t (magic number)
    char buf[4];
    uint32_t magicNumber;
    file.read(buf, 4);
    memcpy(&magicNumber, buf, 4);
    file.close();

    // If file is a PCAP, return that fact
    if (magicNumber == 0xA1B2C3D4) {
        return PCAP;
    }

    // Check if file is a packet file or an analyzer file
    file = std::ifstream(path);
    if (!file.is_open()) {
        throw std::invalid_argument("File does not exist.");
    }

    std::string line;
    if (!std::getline(file, line)) {
        throw std::invalid_argument("File has no content.");
    }

    if (line.find("# PACKETS") != std::string::npos) {
        return CUSTOM_PACKET_FORMAT;
    } else if (line.find("# ANALYZERS") != std::string::npos) {
        return ANALYZER;
    }

    return UNKNOWN;
}

template<class T>
identifier_t InputReader::extractWiFiTypeSubtypeIdentifier(T *pdu) {
    return (pdu->type() << 4u) + pdu->subtype();
}

//std::vector<MyPacket> InputReader::readPCAP(const std::string &path, const std::string &target) {
//    std::vector<MyPacket> packets;
//
//    // LIBTINS
//    Tins::FileSniffer sniffer(path);
//    std::string last_ts;
//    for (auto& currentPacket : sniffer) {
//        MyPacket packet;
//        last_ts = std::to_string(currentPacket.timestamp().seconds()) + "." + std::to_string(currentPacket.timestamp().microseconds());
//#if DEBUG == 1
//        if (packet.getNumber() % 10000 == 0) {
//#endif
//#if DEBUG > 0
//        auto test = currentPacket.pdu()->serialize();
//        std::cout << "Packet " << std::setw(5) << std::setfill(' ') << packet.getNumber() << ":\tts="
//                  << currentPacket.timestamp().seconds() << "." << currentPacket.timestamp().microseconds() << " | "
//                  << std::hex;
//        for (size_t i = 0; i < currentPacket.pdu()->size(); i++) {
//            if (i < 10 || i > currentPacket.pdu()->size() - 10) {
//                std::cout << std::setw(2) << std::setfill('0') << static_cast<uint32_t>(test[i]) << " ";
//            }
//            if (i == 10) {
//                std::cout << " ... ";
//            }
//        }
//        std::cout << std::dec << std::endl;
//#endif
//#if DEBUG == 1
//        }
//#endif
//        for (auto& pdu : Tins::iterate_pdus(currentPacket)) {
//            bool done = false;
//            switch(pdu.pdu_type()) {
//                case Tins::PDU::PDUType::ETHERNET_II: {
//                    packet.addIdentifier(1);
//
//                    // Only add if it is not VLAN
//                    uint16_t ethertype = pdu.find_pdu<Tins::EthernetII>()->payload_type();
//                    if (ethertype != 0x8100) {
//                        packet.addIdentifier(ethertype);
//                    }
//                    break;
//                }
//                case Tins::PDU::PDUType::RADIOTAP: {
//                    // Add radiotap and 802.11 identifier. Radiotap analyzer then returns constant 105 as id
//                    // Because there has to be a 802.11 header after the radio tap header
//                    // 802.11 analyzer then extracts the frame control field for more exact analyzing
//                    packet.addIdentifier(127);
//                    packet.addIdentifier(105);
//                    break;
//                }
//                case Tins::PDU::PDUType::DOT11_QOS_DATA: {
//                    auto* p = pdu.find_pdu<Tins::Dot11QoSData>();
//                    size_t id = extractWiFiTypeSubtypeIdentifier(p);
//                    if (id == 0x2c) {
//                        // Ignore NULL data, has no payload
//                        break;
//                    }
//                    packet.addIdentifier(id);
//                    break;
//                }
//                case Tins::PDU::PDUType::DOT11_DATA: {
//                    auto* p = pdu.find_pdu<Tins::Dot11Data>();
//                    size_t id = extractWiFiTypeSubtypeIdentifier(p);
//                    if (id == 0x24) {
//                        // Ignore NULL data, has no payload
//                        break;
//                    }
//                    packet.addIdentifier(id);
//                    break;
//                }
//                case Tins::PDU::PDUType::DOT11_ASSOC_REQ: {
//                    auto* p = pdu.find_pdu<Tins::Dot11AssocRequest>();
//                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
//                    break;
//                }
//                case Tins::PDU::PDUType::DOT11_ASSOC_RESP: {
//                    auto* p = pdu.find_pdu<Tins::Dot11AssocResponse>();
//                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
//                    break;
//                }
//                case Tins::PDU::PDUType::DOT11_AUTH: {
//                    auto* p = pdu.find_pdu<Tins::Dot11Authentication>();
//                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
//                    break;
//                }
//                case Tins::PDU::PDUType::DOT11_BEACON: {
//                    auto* p = pdu.find_pdu<Tins::Dot11Beacon>();
//                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
//                    break;
//                }
//                case Tins::PDU::PDUType::DOT11_DEAUTH: {
//                    auto* p = pdu.find_pdu<Tins::Dot11Deauthentication>();
//                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
//                    break;
//                }
//                case Tins::PDU::PDUType::DOT11_DIASSOC: {
//                    auto* p = pdu.find_pdu<Tins::Dot11Disassoc>();
//                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
//                    break;
//                }
//                case Tins::PDU::PDUType::DOT11_PROBE_REQ: {
//                    auto* p = pdu.find_pdu<Tins::Dot11ProbeRequest>();
//                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
//                    break;
//                }
//                case Tins::PDU::PDUType::DOT11_PROBE_RESP: {
//                    auto* p = pdu.find_pdu<Tins::Dot11ProbeResponse>();
//                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
//                    break;
//                }
//                case Tins::PDU::PDUType::DOT11_REASSOC_REQ: {
//                    auto* p = pdu.find_pdu<Tins::Dot11ReAssocRequest>();
//                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
//                    break;
//                }
//                case Tins::PDU::PDUType::DOT11_REASSOC_RESP: {
//                    auto* p = pdu.find_pdu<Tins::Dot11ReAssocResponse>();
//                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
//                    break;
//                }
////                case Tins::PDU::PDUType::DOT11_ACK: {
////                    auto* p = pdu.find_pdu<Tins::Dot11Ack>();
////                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
////                    break;
////                }
////                case Tins::PDU::PDUType::DOT11_BLOCK_ACK: {
////                    auto* p = pdu.find_pdu<Tins::Dot11BlockAck>();
////                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
////                    break;
////                }
////                case Tins::PDU::PDUType::DOT11_BLOCK_ACK_REQ: {
////                    auto* p = pdu.find_pdu<Tins::Dot11BlockAckRequest>();
////                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
////                    break;
////                }
////                case Tins::PDU::PDUType::DOT11_CF_END: {
////                    auto* p = pdu.find_pdu<Tins::Dot11CFEnd>();
////                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
////                    break;
////                }
////                case Tins::PDU::PDUType::DOT11_END_CF_ACK: {
////                    auto* p = pdu.find_pdu<Tins::Dot11EndCFAck>();
////                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
////                    break;
////                }
////                case Tins::PDU::PDUType::DOT11_PS_POLL: {
////                    auto* p = pdu.find_pdu<Tins::Dot11PSPoll>();
////                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
////                    break;
////                }
////                case Tins::PDU::PDUType::DOT11_RTS: {
////                    auto* p = pdu.find_pdu<Tins::Dot11RTS>();
////                    packet.addIdentifier(extractWiFiTypeSubtypeIdentifier(p));
////                    break;
////                }
//                case Tins::PDU::PDUType::LLC: {
//                    auto* p = pdu.find_pdu<Tins::LLC>();
//
//                    // Identifier is in SNAP, not in LLC
//                    if ((p->ssap() == 0xAA || p->ssap() == 0xAB) && (p->dsap() == 0xAA || p->dsap() == 0xAB)) {
//                        break;
//                    }
//
//                    // Otherwise no SNAP following, can't decode that proprietary / aged shit anyway
//                    done = true;
//                    break;
//                }
//                case Tins::PDU::PDUType::SNAP: {
//                    auto* p = pdu.find_pdu<Tins::SNAP>();
//
//                    if ((p->ssap() == 0xAA || p->ssap() == 0xAB) && (p->dsap() == 0xAA || p->dsap() == 0xAB)) {
//                        // It is really SNAP, extract identifier
//                        packet.addIdentifier(p->eth_type());
//                    } else {
//                       // Otherwise it is not really SNAP but raw LLC, can't decode that proprietary / aged shit anyway
//                        done = true;
//                    }
//
//                    break;
//                }
//                case Tins::PDU::PDUType::IP: {
//                    packet.addIdentifier(pdu.find_pdu<Tins::IP>()->protocol());
//                    break;
//                }
//                case Tins::PDU::PDUType::IPv6: {
//                    auto* ipv6PDU = pdu.find_pdu<Tins::IPv6>();
//                    Tins::IPv6::next_headers_type nhs = ipv6PDU->next_headers();
//                    // The "next header" field of the last extension header is the identifier if nhs.size() > 0,
//                    // otherwise the identifier is just pdu.next_header
//                    if (!nhs.empty()) {
//                        packet.addIdentifier(nhs.at(nhs.size() - 1));
//                    } else {
//                        packet.addIdentifier(ipv6PDU->next_header());
//                    }
//                    break;
//                }
//                case Tins::PDU::PDUType::DOT1Q: {
//                    packet.addIdentifier(pdu.find_pdu<Tins::Dot1Q>()->payload_type());
//                    break;
//                }
//                case Tins::PDU::PDUType::PPPOE: {
//                    // Ignored because no identifier, the identifier is in PPP for PPPoE sessions
//                    break;
//                }
//                case Tins::PDU::PDUType::PPP: {
//                    packet.addIdentifier(pdu.find_pdu<Tins::PPP>()->protocol());
//                    break;
//                }
//                default: {
//                    // Not supported or no contained identifier payload protocol
//                    // Loopback not supported for now
//                    // 802.3 not supported for now
//                    // DOT11_Management, Dot11_Control, and DOT11 only appear when no subtype found, ignored
//                    // EAPOL, ARP, TCP, UDP, ICMP(v6), BOOTP, DHCP, DNS and many others contain no payload identifier
//                    packet.setPayload("A");
//                    done = true;
//                    break;
//                }
//            }
//
//            if (done) {
//                break;
//            }
//        }
//        // Drop packets that only contain 7f 69 (radiotap 802.11) or only 01 (Ethernet)
//        if ((packet.getIdentifiers().size() == 2 && packet.getIdentifiers()[0] == 0x7f && packet.getIdentifiers()[1] == 0x69) ||
//            (packet.getIdentifiers().size() == 1 && packet.getIdentifiers()[0] == 0x1)) {
//            continue;
//        }
//        packets.push_back(packet);
//    }
//
//    if (!target.empty()) {
//        std::ofstream output;
//        output.open(target);
//        if (!output.is_open()) {
//            std::cerr << "Error occured while opening target file." << std::endl;
//        }
//        std::cout << "Dumping packets to file " << target << std::endl;
//
//        for (const auto &packet : packets) {
//            output << packet;
//        }
//        output.close();
//    }
//
//    return packets;
//}

std::vector<MyPacket> InputReader::readCustomFileFormat(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::invalid_argument("File path does not exist.");
    }

    std::string line;
    size_t lineCounter = 0;
    std::vector<MyPacket> packets;
    while (std::getline(file, line)) {
        lineCounter++;
        if (line.empty()) {
            throw std::invalid_argument("Line " + std::to_string(lineCounter) + " is empty.");
        }

        // Skip first line with the marker
        if (lineCounter == 1) {
            continue;
        }

        MyPacket packet;
        std::istringstream iss(line);
        std::vector<std::string> fields(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());
        if (fields.empty()) {
            throw std::invalid_argument("Line " + std::to_string(lineCounter) + ": Packet is empty.");
        } else if (fields.size() == 1) {
            throw std::invalid_argument("Line " + std::to_string(lineCounter) + ": Packet contains only a payload.");
        }

        for (size_t i = 0; i < fields.size(); i++) {
            std::string &field = fields[i];

            // Strip leading 0x if it exists
            if (field.substr(0, 2) == "0x") {
                field.replace(0, 2, "");
            }

            // Last token in a line is the payload
            if (i == fields.size() - 1) {
                try {
                    packet.setPayload(field);
                } catch (std::invalid_argument &e) {
                    throw std::invalid_argument(
                            "Line " + std::to_string(lineCounter) +
                            ": Payload contains invalid characters (only hex allowed).");
                }
            } else {
                packet.addIdentifier(parseIdentifier(field, lineCounter));
            }
        }

        packets.push_back(packet);
    }

    return packets;
}

identifier_t InputReader::parseIdentifier(const std::string &field, size_t currentLineNum) {
    // 2 chars of a hex string == 1 byte
    if (MyPacket::getMaxIdentifierSize() < field.length() / 2) {
        throw std::invalid_argument(
                "Line " + std::to_string(currentLineNum) +
                ": Identifier " + field +
                " has more than " + std::to_string(MyPacket::getMaxIdentifierSize()) +
                " bytes.");
    }

    try {
        return std::stoull(field, nullptr, 16);
    } catch (std::invalid_argument &e) {
        throw std::invalid_argument(
                "Line " + std::to_string(currentLineNum) +
                ": Identifier " + field +
                " contains invalid characters (only hex allowed).");
    }
}