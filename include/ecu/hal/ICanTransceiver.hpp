#pragma once

#include "ecu/proto/CanFrame.hpp"

namespace ecu::hal {

enum class CanStatus : uint8_t {
    Ok = 0,
    ErrorTxTimeout,
    ErrorTxMailboxFull,
    ErrorBusOff
};

class ICanTransceiver {
public:
    virtual ~ICanTransceiver() = default;

    // It outputs the CAN frame to the hardware line.
    [[nodiscard]] virtual CanStatus transmit(const proto::CanFrame& frame) noexcept = 0;

    // Reads frame from hardware pipeline
    [[nodiscard]] virtual bool receive(proto::CanFrame& frame) noexcept = 0;

    // Even a check to see if there is data.
    [[nodiscard]] virtual bool isBusActive() const noexcept = 0;
};

} // namespace ecu::hal