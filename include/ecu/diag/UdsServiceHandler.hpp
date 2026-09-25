#pragma once

#include <cstdint>
#include <cstring>

#include "ecu/diag/UdsTypes.hpp"
#include "ecu/proto/CanCodec.hpp"

namespace ecu::diag {

class UdsServiceHandler {
public:
    UdsServiceHandler() noexcept = default;

    // Processes the incoming UDS request and populates the response message.
    bool processRequest(const UdsMessage& request,
                        UdsMessage& response,
                         const proto::VehicleTelemetry& currentTelemetry) noexcept {
        if (request.length == 0) {
            return false;
        }

        const auto sid = static_cast<UdsServiceId>(request.payload[0]);

        switch (sid) {
            case UdsServiceId::TesterPresent: // 0x3E
                handleTesterPresent(request, response);
                return true;
            
            case UdsServiceId::ReadDataByIdentifier: // 0x22
                handleReadDataByIdentifier(request, response, currentTelemetry);
                return true;

            default:
                // Unsupported service: 0x7F | SID | ServiceNotSupported (0x11)
                buildNegativeResponse(request.payload[0],
                                      UdsNegativeResponseCode::ServiceNotSupported,
                                      response);
                return true;
        }
    }
private:
    // SID 0x3E: Tester Present (Keeping the ECU connection active)
    void handleTesterPresent(const UdsMessage& request, UdsMessage& response) noexcept {
        if (request.length < 2) {
            buildNegativeResponse(static_cast<uint8_t>(UdsServiceId::TesterPresent),
                                  UdsNegativeResponseCode::IncorrectMessageLengthOrFormat,
                                  response);
            return;
        }

        // Positive Response: 0x7E | SubFunction
        response.payload[0] = static_cast<uint8_t>(static_cast<uint8_t>(UdsServiceId::TesterPresent) + 0x40);
        response.payload[1] = request.payload[1];
        response.length = 2;
    }

    // SID 0X22: Read Data By Identifier
    void handleReadDataByIdentifier(const UdsMessage& request,
                                   UdsMessage& response,
                                   const proto::VehicleTelemetry& telemetry) noexcept {
        // Must be at least SID + 2 bytes DID = 3 bytes
        if (request.length < 3) {
            buildNegativeResponse(static_cast<uint8_t>(UdsServiceId::ReadDataByIdentifier),
                                  UdsNegativeResponseCode::IncorrectMessageLengthOrFormat,
                                  response);
            return;
        }

        // DID Big-Endian (Motorola) merger
        const uint16_t requestedDid = static_cast<uint16_t>(
            (static_cast<uint16_t>(request.payload[1]) << 8) |
            static_cast<uint16_t>(request.payload[2])
        );

        switch (requestedDid) {
            case DataIdentifier::VIN: { // 0xF190 (17-byte chassis number)
                constexpr char vin[] = "WVWZZZ3CZWE123456";
                constexpr uint8_t vinLen = 17;

                response.payload[0] = static_cast<uint8_t>(static_cast<uint8_t>(UdsServiceId::ReadDataByIdentifier) + 0x40); // 0x62
                response.payload[1] = request.payload[1];
                response.payload[2] = request.payload[2];
                std::memcpy(&response.payload[3], vin, vinLen);
                response.length = 3 + vinLen;
                break;
            }

            case DataIdentifier::ECU_SOFTWARE_VERSION: { //0xF189
                constexpr char swVer[] = "SW_v1.0.0_REL";
                constexpr uint8_t swLen = 13;

                response.payload[0] = static_cast<uint8_t>(static_cast<uint8_t>(UdsServiceId::ReadDataByIdentifier) + 0x40);
                response.payload[1] = request.payload[1];
                response.payload[2] = request.payload[2];
                std::memcpy(&response.payload[3], swVer, swLen);
                response.length = 3 + swLen;
                break;
            }

            case DataIdentifier::CURRENT_TELEMETRY: { // 0x2001 (Live Telemetry Package)
                response.payload[0] = static_cast<uint8_t>(static_cast<uint8_t>(UdsServiceId::ReadDataByIdentifier) + 0x40);
                response.payload[1] = request.payload[1];
                response.payload[2] = request.payload[2];

                // Encode the existing telemetry as 8 bytes via the codec.
                const proto::CanFrame frame = proto::CanCodec::encodeTelemetry(telemetry);
                std::memcpy(&response.payload[3], frame.data.data(), frame.dlc);
                response.length = static_cast<uint8_t>(3 + frame.dlc);
                break;
            }

            default:
                // Unsupported DID: RequestOutOfRange (0x31)
                buildNegativeResponse(static_cast<uint8_t>(UdsServiceId::ReadDataByIdentifier),
                                      UdsNegativeResponseCode::RequestOutOfRange,
                                      response);
                break;
        }
    }

    void buildNegativeResponse(uint8_t requestedSid,
                              UdsNegativeResponseCode nrc,
                              UdsMessage& response) noexcept {
        response.payload[0] = static_cast<uint8_t>(UdsServiceId::NegativeResponse); // 0x7F
        response.payload[1] = requestedSid;
        response.payload[2] = static_cast<uint8_t>(nrc);
        response.length = 3;
    }
};

} // namespace ecu::diag