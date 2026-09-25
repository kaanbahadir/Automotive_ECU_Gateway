#pragma once

#include <cstdint>
#include "ecu/proto/CanCodec.hpp"

namespace ecu::app {

class VehicleDynamicsModel {
public:
    VehicleDynamicsModel() noexcept = default;

    // 20 ms control step (50 Hz)
    void step(bool throttlePressed, bool brakePressed) noexcept {
        updateEngineAndSpeed(throttlePressed, brakePressed);
        updateCoolantTemperature();
        updateBatteryVoltage();
        
        aliveCounter_ = static_cast<uint8_t>((aliveCounter_ + 1U) & 0x0FU);
    }

    [[nodiscard]] proto::VehicleTelemetry getTelemetrySnapshot() const noexcept {
        proto::VehicleTelemetry telemetry{};
        telemetry.engineRpm = engineRpm_;
        telemetry.vehicleSpeed = vehicleSpeed_;
        telemetry.coolantTemp = coolantTemp_;
        telemetry.batteryVoltage = batteryVoltage_;
        telemetry.aliveCounter = aliveCounter_;
        return telemetry;
    }

private:
    void updateEngineAndSpeed(bool throttle, bool brake) noexcept {
        if (throttle && !brake) {
            if (engineRpm_ < MAX_RPM - RPM_ACCEL_STEP) {
                engineRpm_ += RPM_ACCEL_STEP;
            } else {
                engineRpm_ = MAX_RPM;
            }

            if (vehicleSpeed_ < MAX_SPEED - SPEED_ACCEL_STEP) {
                vehicleSpeed_ += SPEED_ACCEL_STEP;
            } else {
                vehicleSpeed_ = MAX_SPEED;
            }
        } else if (brake) {
            if (engineRpm_ > IDLE_RPM + (RPM_DECEL_STEP * 2)) {
                engineRpm_ -= (RPM_DECEL_STEP * 2);
            } else {
                engineRpm_ = IDLE_RPM;
            }

            if (vehicleSpeed_ > SPEED_DECEL_STEP * 2) {
                vehicleSpeed_ -= (SPEED_DECEL_STEP * 2);
            } else {
                vehicleSpeed_ = 0;
            }
        } else {
            // Engine braking when there is no gas/brake.
            if (engineRpm_ > IDLE_RPM + RPM_DECEL_STEP) {
                engineRpm_ -= RPM_DECEL_STEP;
            } else {
                engineRpm_ = IDLE_RPM;
            }

            if (vehicleSpeed_ > SPEED_DECEL_STEP) {
                vehicleSpeed_ -= SPEED_DECEL_STEP;
            } else {
                vehicleSpeed_ = 0;
            }
        }
    }

    void updateCoolantTemperature() noexcept {
        // At high RPM, the temperature rises slightly; it normally stabilizes around 90°C.
        if (engineRpm_ > 3500 && coolantTemp_ < 105) {
            coolantTemp_ += 1;
        } else if (engineRpm_ <= 2000 && coolantTemp_ > 88) {
            coolantTemp_ -= 1;
        }
    }

    void updateBatteryVoltage() noexcept {
        // Alternator regulation simulation: 13.8V - 14.2V range (0.1V scale = 138 - 142)
        batteryVoltage_ = static_cast<uint8_t>(138U + (aliveCounter_ % 5U));
    }

    static constexpr uint16_t IDLE_RPM = 800;
    static constexpr uint16_t MAX_RPM = 6500;
    static constexpr uint16_t RPM_ACCEL_STEP = 50;
    static constexpr uint16_t RPM_DECEL_STEP = 25;

    static constexpr uint16_t MAX_SPEED = 22000; // 220.00 km/h
    static constexpr uint16_t SPEED_ACCEL_STEP = 40; // ~0.4 km/h / 20ms
    static constexpr uint16_t SPEED_DECEL_STEP = 20;

    uint16_t engineRpm_{IDLE_RPM};
    uint16_t vehicleSpeed_{0};
    int8_t coolantTemp_{90};
    uint8_t batteryVoltage_{140};
    uint8_t aliveCounter_{0};
};

} // namespace ecu::app