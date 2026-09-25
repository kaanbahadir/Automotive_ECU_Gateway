#pragma once

#include <cstddef>
#include <cstdint>
#include "ecu/core/CanRingBuffer.hpp"
#include "ecu/proto/CanCodec.hpp"
#include "ecu/proto/CanFrame.hpp"
#include "ecu/app/VehicleDynamicsModel.hpp"

namespace ecu::tasks {

template <std::size_t QueueCapacity>
class TelemetryTask {
public:
    explicit TelemetryTask(core::CanRingBuffer<QueueCapacity>& outQueue) noexcept
        : outQueue_(outQueue) {}

    // 50 Hz periodic step: Retrieves telemetry from the dynamic model, encodes it, and places it in the queue.
    void step(bool throttle = true, bool brake = false) noexcept {
        vehicleModel_.step(throttle, brake);
        currentTelemetry_ = vehicleModel_.getTelemetrySnapshot();

        const proto::CanFrame frame = proto::CanCodec::encodeTelemetry(currentTelemetry_);

        if (!outQueue_.push(frame)) {
            ++droppedFramesCount_;
        }
    }

    [[nodiscard]] const proto::VehicleTelemetry& getCurrentTelemetry() const noexcept {
        return currentTelemetry_;
    }

    [[nodiscard]] uint32_t getDroppedFramesCount() const noexcept {
        return droppedFramesCount_;
    }

private:
    core::CanRingBuffer<QueueCapacity>& outQueue_;
    app::VehicleDynamicsModel vehicleModel_{};
    proto::VehicleTelemetry currentTelemetry_{};
    uint32_t droppedFramesCount_{0};
};

} // namespace ecu::tasks