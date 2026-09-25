#pragma once

#include <cstdint>
#include <array>

namespace ecu::diag {

// ISO 14229 Standard Service Identifiers (SID)
enum class UdsServiceId : uint8_t {
    DiagnosticSessionControl = 0x10,
    ECUReset                 = 0x11,
    ReadDataByIdentifier     = 0x22,
    WriteDataByIdentifier    = 0x2E,
    TesterPresent            = 0x3E,
    NegativeResponse         = 0x7F
};

// ISO 14229 Negative Response Codes (NRC)
enum class UdsNegativeResponseCode : uint8_t {
    GeneralReject                    = 0x10,
    ServiceNotSupported              = 0x11,
    SubFunctionNotSupported          = 0x12,
    IncorrectMessageLengthOrFormat   = 0x13,
    ConditionsNotCorrect             = 0x22,
    RequestSequenceError             = 0x24,
    RequestOutOfRange                = 0x31,
    SecurityAccessDenied             = 0x33
};

// Data Identifiers (DID) for SID 0x22
namespace DataIdentifier {
    constexpr uint16_t VIN                    = 0xF190; // Vehicle Identification Number
    constexpr uint16_t ECU_SOFTWARE_VERSION   = 0xF189; // Software Version
    constexpr uint16_t CURRENT_TELEMETRY      = 0x2001; // Live Gateway Telemetry snapshot
}

// Fixed-capacity Raw Diagnostic Payload (Zero Heap)
struct UdsMessage {
    uint8_t length{0};
    std::array<uint8_t, 64> payload{}; // Support for small ISO-TP frames
};

} // namespace ecu::diag