#include <cassert>
#include <cstring>
#include <iostream>
#include "ecu/diag/UdsServiceHandler.hpp"
#include "ecu/diag/UdsTypes.hpp"
#include "ecu/proto/CanCodec.hpp"

int main() {
    std::cout << "[RUNNING] UDS Diagnostic Service Unit Test..." << std::endl;

    ecu::diag::UdsServiceHandler handler;
    ecu::proto::VehicleTelemetry liveTelemetry{};
    liveTelemetry.engineRpm = 2500;
    liveTelemetry.vehicleSpeed = 12000; // 120.00 km/h
    liveTelemetry.coolantTemp = 85;
    liveTelemetry.batteryVoltage = 138; // 13.8 V
    liveTelemetry.aliveCounter = 4;

    ecu::diag::UdsMessage request{};
    ecu::diag::UdsMessage response{};

    // 1. Test: Tester Present (SID 0x3E, SubFunction 0x00)
    request.payload[0] = 0x3E;
    request.payload[1] = 0x00;
    request.length = 2;

    assert(handler.processRequest(request, response, liveTelemetry));
    assert(response.length == 2);
    assert(response.payload[0] == 0x7E); // 0x3E + 0x40
    assert(response.payload[1] == 0x00);

    // 2. Test: Read VIN (SID 0x22, DID 0xF190)
    request.payload[0] = 0x22;
    request.payload[1] = 0xF1;
    request.payload[2] = 0x90;
    request.length = 3;

    assert(handler.processRequest(request, response, liveTelemetry));
    assert(response.payload[0] == 0x62); // Positive response: 0x22 + 0x40
    assert(response.payload[1] == 0xF1);
    assert(response.payload[2] == 0x90);
    assert(response.length == 3 + 17); // 3-byte header + 17-byte VIN

    char receivedVin[18]{};
    std::memcpy(receivedVin, &response.payload[3], 17);
    assert(std::strcmp(receivedVin, "WVWZZZ3CZWE123456") == 0);

    // 3. Test: Read Live Telemetry (SID 0x22, DID 0x2001)
    request.payload[0] = 0x22;
    request.payload[1] = 0x20;
    request.payload[2] = 0x01;
    request.length = 3;

    assert(handler.processRequest(request, response, liveTelemetry));
    assert(response.payload[0] == 0x62);
    assert(response.payload[1] == 0x20);
    assert(response.payload[2] == 0x01);
    assert(response.length == 3 + 8); // 3 bytes header + 8 bytes CAN telemetry

    // Decode and verify the telemetry in the response.
    ecu::proto::CanFrame decodedFrame{};
    decodedFrame.id = ecu::proto::CanCodec::TELEMETRY_CAN_ID;
    decodedFrame.dlc = 8;
    std::memcpy(decodedFrame.data.data(), &response.payload[3], 8);

    const auto decodedTelemetry = ecu::proto::CanCodec::decodeTelemetry(decodedFrame);
    assert(decodedTelemetry.engineRpm == 2500);
    assert(decodedTelemetry.vehicleSpeed == 12000);
    assert(decodedTelemetry.coolantTemp == 85);

    // 4. Test: Negative Response - Unknown DID (0x9999) -> NRC 0x31 (RequestOutOfRange)
    request.payload[0] = 0x22;
    request.payload[1] = 0x99;
    request.payload[2] = 0x99;
    request.length = 3;

    assert(handler.processRequest(request, response, liveTelemetry));
    assert(response.length == 3);
    assert(response.payload[0] == 0x7F); // NRC
    assert(response.payload[1] == 0x22); // Requested SID
    assert(response.payload[2] == static_cast<uint8_t>(ecu::diag::UdsNegativeResponseCode::RequestOutOfRange));

    // Test 5: Negative Response - Service Not Supported (0x27 SecurityAccess) -> NRC 0x11
    request.payload[0] = 0x27;
    request.length = 1;

    assert(handler.processRequest(request, response, liveTelemetry));
    assert(response.length == 3);
    assert(response.payload[0] == 0x7F);
    assert(response.payload[1] == 0x27);
    assert(response.payload[2] == static_cast<uint8_t>(ecu::diag::UdsNegativeResponseCode::ServiceNotSupported));

    std::cout << "[PASSED] All UDS diagnostic assertions passed successfully!" << std::endl;
    return 0;
}