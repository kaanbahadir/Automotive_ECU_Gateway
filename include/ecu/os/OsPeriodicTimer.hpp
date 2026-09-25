#pragma once

#include <chrono>
#include <cstdint>
#include <thread>

namespace ecu::os {

template <uint32_t PeriodMs>
class OsPeriodicTimer {
public:
    using Clock = std::chrono::steady_clock;
    using Milliseconds = std::chrono::milliseconds;

    OsPeriodicTimer() noexcept 
        : nextWakeTime_(Clock::now() + Milliseconds(PeriodMs)) {}

    // Similar to FreeRTOS vTaskDelayUntil: Waits until the next periodic interval.
    // It prevents jitter (timing drift) by taking the task's own execution time into account.
    void waitNextPeriod() noexcept {
        const auto now = Clock::now();
        if (now < nextWakeTime_) {
            std::this_thread::sleep_until(nextWakeTime_);
        }
        nextWakeTime_ += Milliseconds(PeriodMs);
        currentTickMs_ += PeriodMs;
    }

    [[nodiscard]] uint32_t getTickMs() const noexcept {
        return currentTickMs_;
    }

    static constexpr uint32_t getPeriodMs() noexcept {
        return PeriodMs;
    }

private:
    std::chrono::time_point<Clock> nextWakeTime_;
    uint32_t currentTickMs_{0};
};

} // namespace ecu::os