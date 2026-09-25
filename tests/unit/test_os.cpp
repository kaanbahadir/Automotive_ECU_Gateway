#include <cassert>
#include <iostream>
#include "ecu/os/OsPeriodicTimer.hpp"

int main() {
    std::cout << "[RUNNING] OSAL (FreeRTOS Simulation Timer) Test..." << std::endl;

    // 20 ms (50 Hz) timer
    ecu::os::OsPeriodicTimer<20> timer;
    assert(timer.getPeriodMs() == 20);
    assert(timer.getTickMs() == 0);

    // Wait for 3 periods (total ~60 ms)
    for (int i = 0; i < 3; ++i) {
        timer.waitNextPeriod();
    }

    assert(timer.getTickMs() == 60);

    std::cout << "[PASSED] OSAL periodic timer assertions passed successfully!" << std::endl;
    return 0;
}