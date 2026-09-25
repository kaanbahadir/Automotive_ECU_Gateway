#include <cassert>
#include <iostream>
#include "ecu/app/VehicleDynamicsModel.hpp"

int main() {
    std::cout << "[RUNNING] Application Layer (Vehicle Dynamics) Test..." << std::endl;

    ecu::app::VehicleDynamicsModel model;

    // Initial status check
    auto snapshot = model.getTelemetrySnapshot();
    assert(snapshot.engineRpm == 800);
    assert(snapshot.vehicleSpeed == 0);
    assert(snapshot.coolantTemp == 90);

    // When the gas pedal is pressed, the RPM and speed should increase.
    for (int i = 0; i < 10; ++i) {
        model.step(true, false);
    }
    snapshot = model.getTelemetrySnapshot();
    assert(snapshot.engineRpm > 800);
    assert(snapshot.vehicleSpeed > 0);

    // When the brake is applied, RPM and speed should decrease.
    const uint16_t currentRpm = snapshot.engineRpm;
    model.step(false, true);
    snapshot = model.getTelemetrySnapshot();
    assert(snapshot.engineRpm < currentRpm);

    std::cout << "[PASSED] Application Layer test completed successfully!" << std::endl;
    return 0;
}