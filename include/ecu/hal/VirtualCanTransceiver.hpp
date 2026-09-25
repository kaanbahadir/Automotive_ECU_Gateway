#pragma once

#include <array>
#include "ecu/hal/ICanTransceiver.hpp"

namespace ecu::hal {

template <std::size_t HardwareBufferDepth = 16>
class VirtualCanTransceiver final : public ICanTransceiver {
public:
    VirtualCanTransceiver() noexcept = default;

    [[nodiscard]] CanStatus transmit(const proto::CanFrame& frame) noexcept override {
        if (txCount_ >= HardwareBufferDepth) {
            return CanStatus::ErrorTxMailboxFull;
        }

        lastTransmittedFrame_ = frame;
        ++totalTransmittedFrames_;
        return CanStatus::Ok;
    }

    [[nodiscard]] bool receive(proto::CanFrame& frame) noexcept override {
        // Loopback / Reading point for incoming simulation messages
        if (hasRxPending_) {
            frame = rxFrame_;
            hasRxPending_ = false;
            return true;
        }
        return false;
    }

    [[nodiscard]] bool isBusActive() const noexcept override {
        return busActive_;
    }

    // Hardware-in-the-loop injection methods for simulation
    void injectRxFrame(const proto::CanFrame& frame) noexcept {
        rxFrame_ = frame;
        hasRxPending_ = true;
    }

    [[nodiscard]] const proto::CanFrame& getLastTransmittedFrame() const noexcept {
        return lastTransmittedFrame_;
    }

    [[nodiscard]] uint32_t getTotalTransmittedCount() const noexcept {
        return totalTransmittedFrames_;
    }

private:
    proto::CanFrame lastTransmittedFrame_{};
    proto::CanFrame rxFrame_{};
    uint32_t totalTransmittedFrames_{0};
    std::size_t txCount_{0};
    bool hasRxPending_{false};
    bool busActive_{true};
};

} // namespace ecu::hal