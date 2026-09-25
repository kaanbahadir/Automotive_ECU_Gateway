#include <cassert>
#include <iostream>
#include "ecu/core/CanRingBuffer.hpp"
#include "ecu/hal/VirtualCanTransceiver.hpp"
#include "ecu/tasks/TelemetryTask.hpp"
#include "ecu/tasks/CanDispatchTask.hpp"

int main() {
    std::cout << "[RUNNING] TelemetryTask & CanDispatchTask Integration Test..." << std::endl;

    constexpr std::size_t QUEUE_SIZE = 5;
    ecu::core::CanRingBuffer<QUEUE_SIZE> sharedQueue;
    ecu::hal::VirtualCanTransceiver<16> transceiver;

    ecu::tasks::TelemetryTask<QUEUE_SIZE> telemetryTask(sharedQueue);
    ecu::tasks::CanDispatchTask<QUEUE_SIZE> dispatchTask(sharedQueue, transceiver);

    // Initial state check
    assert(telemetryTask.getDroppedFramesCount() == 0);
    assert(dispatchTask.getTransmittedFramesCount() == 0);

    // Step 1: TelemetryTask generates a CAN frame and pushes it to the queue
    telemetryTask.step();
    assert(telemetryTask.getDroppedFramesCount() == 0);

    // Step 2: CanDispatchTask pops the frame and transmits it via HAL transceiver
    const bool dispatched = dispatchTask.step();
    assert(dispatched == true);
    assert(dispatchTask.getTransmittedFramesCount() == 1);
    assert(transceiver.getTotalTransmittedCount() == 1);

    // Verify frame contents in transceiver
    const auto& lastFrame = transceiver.getLastTransmittedFrame();
    assert(lastFrame.id == 0x100);
    assert(lastFrame.dlc == 8);

    // Step 3: Queue should now be empty; subsequent step must return false
    assert(dispatchTask.step() == false);

    // Step 4: Overflow simulation
    for (std::size_t i = 0; i < QUEUE_SIZE + 2; ++i) {
        telemetryTask.step();
    }
    assert(telemetryTask.getDroppedFramesCount() > 0);

    std::cout << "[PASSED] Task integration assertions passed successfully!" << std::endl;
    return 0;
}