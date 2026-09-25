#pragma once

#include <cstdint>
#include "CanFrame.hpp"

namespace ecu::proto {
/**
 * Clean engineering vehicle telemetry data.
 */
    struct VehicleTelemetry
    {
        uint16_t engineRpm{0};       // Engine speed (RPM)
        uint16_t vehicleSpeed{0};    // Vehicle speed (0.01 km/h resolution)
        int8_t coolantTemp{0};       // Engine coolant temperature in Celsius (-40 to +120)
        uint8_t batteryVoltage{0};   // Battery voltage in 0.1V units (e.g. 124 = 12.4V)
        uint8_t aliveCounter{0};     // Rolling alive counter (0 to 15)
    };
    class CanCodec {
        public:
            static constexpr uint32_t TELEMETRY_CAN_ID{0x100};

            static CanFrame encodeTelemetry (const VehicleTelemetry& data) {
                CanFrame frame{};
                frame.id = TELEMETRY_CAN_ID;
                frame.dlc = 8;
                frame.isExtended = false;

                // 1. Engine RPM (Bytes 0-1) - High byte then Low byte
                frame.data[0] = static_cast<uint8_t>((data.engineRpm >> 8) & 0xFF);
                frame.data[1] = static_cast<uint8_t>(data.engineRpm & 0xFF);

                // 2. Vehicle Speed (Bytes 2-3) - High byte then Low byte
                frame.data[2] = static_cast<uint8_t>((data.vehicleSpeed >> 8) & 0xFF);
                frame.data[3] = static_cast<uint8_t>(data.vehicleSpeed & 0xFF);

                // 3. Coolant Temperature (Byte 4)
                frame.data[4] = static_cast<uint8_t>(data.coolantTemp);

                // 4. Battery Voltage (Byte 5)
                frame.data[5] = data.batteryVoltage;

                // 5. Alive Counter (Byte 6) - Only lower 4 bits (0-15)
                frame.data[6] = data.aliveCounter & 0x0F;

                // 6. Checksum placeholder (Byte 7)
                frame.data[7] = 0x00;

                return frame;
            }

            static VehicleTelemetry decodeTelemetry(const CanFrame& frame) {
                VehicleTelemetry data{};

                // 1. Engine RPM (Bytes 0-1) - Recombine High and Low bytes
                data.engineRpm = static_cast<uint16_t>((static_cast<uint16_t>(frame.data[0]) << 8) | frame.data[1]);

                // 2. Vehicle Speed (Bytes 2-3) - Recombine High and Low bytes
                data.vehicleSpeed = static_cast<uint16_t>((static_cast<uint16_t>(frame.data[2]) << 8) | frame.data[3]);

                // 3. Coolant Temperature (Byte 4) - Cast back to signed int8_t
                data.coolantTemp = static_cast<int8_t>(frame.data[4]);

                // 4. Battery Voltage (Byte 5)
                data.batteryVoltage = frame.data[5];

                // 5. Alive Counter (Byte 6) - Extract lower 4 bits
                data.aliveCounter = frame.data[6] & 0x0F;

                return data;
            }
    };
} // namespace ecu::proto