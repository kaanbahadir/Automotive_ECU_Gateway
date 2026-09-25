#pragma once

#include <cstddef>
#include <cstdint>
#include "ecu/core/CanRingBuffer.hpp"
#include "ecu/hal/ICanTransceiver.hpp"
#include "ecu/proto/CanFrame.hpp"

namespace ecu::tasks {

template <std::size_t QueueCapacity>
class CanDispatchTask {
public:
    explicit CanDispatchTask(core::CanRingBuffer<QueueCapacity>& inQueue,
                             hal::ICanTransceiver& transceiver) noexcept
        : inQueue_(inQueue),
          transceiver_(transceiver) {}

    // Consumer loop step: Retrieves the frame from the queue and transmits it to the CAN hardware.
    [[nodiscard]] bool step() noexcept {
        proto::CanFrame frame{};
        const bool hasFrame = inQueue_.pop(frame);

        if (!hasFrame) {
            return false;
        }

        // Even via the hardware abstraction layer (HAL).
        const hal::CanStatus status = transceiver_.transmit(frame);
        if (status == hal::CanStatus::Ok) {
            lastTransmittedFrame_ = frame;
            ++transmittedFramesCount_;
            return true;
        }

        // Hardware transmission error (TX Mailbox Full / Timeout, etc.)
        ++transmissionErrorsCount_;
        return false;
    }

    [[nodiscard]] uint32_t getTransmittedFramesCount() const noexcept {
        return transmittedFramesCount_;
    }

    [[nodiscard]] uint32_t getTransmissionErrorsCount() const noexcept {
        return transmissionErrorsCount_;
    }

    [[nodiscard]] const proto::CanFrame& getLastTransmittedFrame() const noexcept {
        return lastTransmittedFrame_;
    }

private:
    core::CanRingBuffer<QueueCapacity>& inQueue_;
    hal::ICanTransceiver& transceiver_;
    proto::CanFrame lastTransmittedFrame_{};
    uint32_t transmittedFramesCount_{0};
    uint32_t transmissionErrorsCount_{0};
};

} // namespace ecu::tasks