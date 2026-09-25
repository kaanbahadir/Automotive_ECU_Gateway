#pragma once

#include <cstdint>
#include <array>

namespace ecu::proto {
/**
 * Standard CAN 2.0B frame structure.
 * Fixed 8-byte payload layout (no dynamic allocation).
 */
    struct CanFrame {
       uint32_t id{0};                // 11-bit standard or 29-bit extended arbitration ID
       uint8_t dlc{0};                // Payload length code (0 to 8 bytes)
       bool isExtended{false};        // Extended identifier flag
       std::array<uint8_t, 8> data{}; // Raw payload buffer

    };    
} // namespace ecu::proto