#include <cassert>
#include <iostream>
#include "ecu/proto/CanCodec.hpp"

int main() {
    std::cout << "[RUNNING] CAN Telemetry Codec Unit Test..." << std::endl;

    // 1. Generating Original Telemetry Data
    ecu::proto::VehicleTelemetry original{};
    original.engineRpm = 3500;        // 3500 RPM
    original.vehicleSpeed = 12050;    // 120.50 km/h
    original.coolantTemp = -15;       // -15 °C (Negatif sıcaklık testi)
    original.batteryVoltage = 126;    // 12.6 V
    original.aliveCounter = 9;        // Sayaç değeri

    // 2. Packaging (Encoding)
    ecu::proto::CanFrame frame = ecu::proto::CanCodec::encodeTelemetry(original);

    // CAN ID and DLC Checks
    assert(frame.id == ecu::proto::CanCodec::TELEMETRY_CAN_ID);
    assert(frame.dlc == 8);
    assert(!frame.isExtended);

    // 3. Decoding
    ecu::proto::VehicleTelemetry decoded = ecu::proto::CanCodec::decodeTelemetry(frame);

    // 4. Verification (Assert – Are the beginning and the end equal?)
    assert(decoded.engineRpm == original.engineRpm);
    assert(decoded.vehicleSpeed == original.vehicleSpeed);
    assert(decoded.coolantTemp == original.coolantTemp);
    assert(decoded.batteryVoltage == original.batteryVoltage);
    assert(decoded.aliveCounter == original.aliveCounter);

    std::cout << "[PASSED] All telemetry codec assertions passed successfully!" << std::endl;
    return 0;
}