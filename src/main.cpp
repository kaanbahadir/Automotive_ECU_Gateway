#include <iostream>
#include <iomanip>
#include <cstddef>

#include "ecu/core/CanRingBuffer.hpp"
#include "ecu/hal/VirtualCanTransceiver.hpp"
#include "ecu/os/OsPeriodicTimer.hpp"
#include "ecu/tasks/TelemetryTask.hpp"
#include "ecu/tasks/CanDispatchTask.hpp"
#include "ecu/diag/UdsServiceHandler.hpp"

void printCanalyzerFormat(uint32_t timestampMs, const ecu::proto::CanFrame& frame) {
    std::cout << std::dec << std::setw(6) << std::setfill(' ') << timestampMs << " ms | ";
    std::cout << "CAN TX | ID: 0x" << std::hex << std::uppercase << std::setw(3) << std::setfill('0') << frame.id;
    std::cout << " | DLC: " << std::dec << static_cast<int>(frame.dlc) << " | Data: ";
    
    for (std::size_t i = 0; i < frame.dlc; ++i) {
        std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(frame.data[i]) << " ";
    }
    std::cout << std::endl;
}

void printUdsTrace(uint32_t timestampMs, const char* dir, const ecu::diag::UdsMessage& msg) {
    std::cout << std::dec << std::setw(6) << std::setfill(' ') << timestampMs << " ms | ";
    std::cout << "UDS " << dir << " | LEN: " << std::dec << static_cast<int>(msg.length) << " | Payload: ";
    for (std::size_t i = 0; i < msg.length; ++i) {
        std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                  << static_cast<int>(msg.payload[i]) << " ";
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "=================================================================\n";
    std::cout << " Automotive ECU Gateway Simulation (Full AUTOSAR-Style Stack)   \n";
    std::cout << "=================================================================\n\n";

    constexpr std::size_t QUEUE_SIZE = 10;
    ecu::core::CanRingBuffer<QUEUE_SIZE> sharedQueue;
    ecu::hal::VirtualCanTransceiver<16> canTransceiver;

    ecu::tasks::TelemetryTask<QUEUE_SIZE> telemetryTask(sharedQueue);
    ecu::tasks::CanDispatchTask<QUEUE_SIZE> dispatchTask(sharedQueue, canTransceiver);
    ecu::diag::UdsServiceHandler udsHandler;

    ecu::os::OsPeriodicTimer<20> osTimer;

    constexpr int SIMULATION_STEPS = 50;

    std::cout << "System Boot Complete. Tasks Running via HAL & OSAL...\n\n";

    for (int step = 0; step < SIMULATION_STEPS; ++step) {
        const uint32_t currentTick = osTimer.getTickMs();

        const bool throttle = (step < 30);
        const bool brake = (step >= 30);

        telemetryTask.step(throttle, brake);

        while (dispatchTask.step()) {
            const auto& txFrame = canTransceiver.getLastTransmittedFrame();
            printCanalyzerFormat(currentTick, txFrame);
        }

        if (currentTick == 200) {
            ecu::diag::UdsMessage req{2, {0x3E, 0x00}};
            ecu::diag::UdsMessage res{};
            printUdsTrace(currentTick, "REQ", req);
            udsHandler.processRequest(req, res, telemetryTask.getCurrentTelemetry());
            printUdsTrace(currentTick, "RES", res);
        } else if (currentTick == 400) {
            ecu::diag::UdsMessage req{3, {0x22, 0xF1, 0x90}};
            ecu::diag::UdsMessage res{};
            printUdsTrace(currentTick, "REQ", req);
            udsHandler.processRequest(req, res, telemetryTask.getCurrentTelemetry());
            printUdsTrace(currentTick, "RES", res);
        } else if (currentTick == 600) {
            ecu::diag::UdsMessage req{3, {0x22, 0x20, 0x01}};
            ecu::diag::UdsMessage res{};
            printUdsTrace(currentTick, "REQ", req);
            udsHandler.processRequest(req, res, telemetryTask.getCurrentTelemetry());
            printUdsTrace(currentTick, "RES", res);
        } else if (currentTick == 800) {
            ecu::diag::UdsMessage req{1, {0x99}};
            ecu::diag::UdsMessage res{};
            printUdsTrace(currentTick, "REQ", req);
            udsHandler.processRequest(req, res, telemetryTask.getCurrentTelemetry());
            printUdsTrace(currentTick, "RES", res);
        }

        osTimer.waitNextPeriod();
    }

    std::cout << "\n=================================================================\n";
    std::cout << " Simulation Complete! Hardware & OS Metrics:\n";
    std::cout << " Operating System Ticks (ms) : " << std::dec << osTimer.getTickMs() << " ms\n";
    std::cout << " Transceiver Total TX Frames : " << canTransceiver.getTotalTransmittedCount() << "\n";
    std::cout << " Dispatcher Transmission OK : " << dispatchTask.getTransmittedFramesCount() << "\n";
    std::cout << " Dispatcher Errors (Bus/HW) : " << dispatchTask.getTransmissionErrorsCount() << "\n";
    std::cout << " Queue Dropped Frames       : " << telemetryTask.getDroppedFramesCount() << "\n";
    std::cout << "=================================================================\n";

    return 0;
}