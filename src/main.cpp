#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <atomic>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "ecu/app/vehicle_dynamics.hpp"
#include "ecu/tasks/telemetry_task.hpp"
#include "ecu/tasks/can_dispatch_task.hpp"
#include "ecu/diag/uds_service_handler.hpp"
#include "ecu/hal/virtual_can_transceiver.hpp"

// Non-blocking keyboard input helper for POSIX (macOS / Linux)
class KeyboardController {
public:
    KeyboardController() {
        tcgetattr(STDIN_FILENO, &orig_termios_);
        struct termios raw = orig_termios_;
        raw.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }
    ~KeyboardController() {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios_);
    }
    char readKey() {
        char ch = 0;
        if (read(STDIN_FILENO, &ch, 1) > 0) {
            return ch;
        }
        return 0;
    }
private:
    struct termios orig_termios_;
};

void drawBar(std::string_view label, double val, double max_val, int width, const std::string& color) {
    int filled = static_cast<int>((val / max_val) * width);
    if (filled > width) filled = width;
    if (filled < 0) filled = 0;

    std::cout << "  " << std::setw(8) << std::left << label << " [";
    std::cout << color;
    for (int i = 0; i < filled; ++i) std::cout << "#";
    std::cout << "\033[0m";
    for (int i = filled; i < width; ++i) std::cout << " ";
    std::cout << "] " << std::setw(7) << std::right << std::fixed << std::setprecision(1) << val;
}

int main() {
    using namespace ecu;

    hal::VirtualCanTransceiver transceiver;
    core::CanRingBuffer<16> can_fifo;
    app::VehicleDynamics dynamics;
    diag::UdsServiceHandler uds_handler;

    tasks::TelemetryTask telemetry_task(dynamics, can_fifo);
    tasks::CanDispatchTask dispatch_task(can_fifo, transceiver);

    KeyboardController keyboard;
    std::atomic<bool> running{true};

    std::string last_uds_req = "None";
    std::string last_uds_res = "None";
    int throttle_level = 0;
    int brake_level = 0;
    uint32_t loop_count = 0;

    // Clear Screen & Hide Cursor
    std::cout << "\033[2J\033[?25l";

    while (running.load()) {
        char key = keyboard.readKey();
        if (key == 'q' || key == 'Q') {
            running.store(false);
        } else if (key == 'w' || key == 'W') {
            throttle_level = std::min(throttle_level + 15, 100);
            brake_level = 0;
        } else if (key == 's' || key == 'S') {
            brake_level = std::min(brake_level + 20, 100);
            throttle_level = 0;
        } else if (key == 'a' || key == 'A') {
            throttle_level = std::max(throttle_level - 10, 0);
            brake_level = std::max(brake_level - 10, 0);
        } else if (key == '1') {
            // UDS Tester Present (0x3E 0x00)
            const uint8_t req[] = {0x3E, 0x00};
            uint8_t res[8] = {0};
            size_t res_len = 0;
            uds_handler.processRequest(req, 2, res, sizeof(res), res_len);
            last_uds_req = "0x3E 0x00 (Tester Present)";
            last_uds_res = "0x7E 0x00 (Positive Response)";
        } else if (key == '2') {
            // UDS Read VIN (0x22 0xF1 0x90)
            const uint8_t req[] = {0x22, 0xF1, 0x90};
            uint8_t res[32] = {0};
            size_t res_len = 0;
            uds_handler.processRequest(req, 3, res, sizeof(res), res_len);
            last_uds_req = "0x22 0xF1 0x90 (Read VIN)";
            last_uds_res = "0x62 0xF1 0x90 [WVWZZZ3CZWE123456]";
        } else if (key == '3') {
            // UDS Telemetry Snapshot (0x22 0x20 0x01)
            const uint8_t req[] = {0x22, 0x20, 0x01};
            uint8_t res[32] = {0};
            size_t res_len = 0;
            uds_handler.processRequest(req, 3, res, sizeof(res), res_len);
            last_uds_req = "0x22 0x20 0x01 (Live Telemetry DID)";
            last_uds_res = "0x62 0x20 0x01 [Payload Snapshot OK]";
        } else if (key == '4') {
            // UDS Invalid Service (0x99 -> NRC 0x11)
            const uint8_t req[] = {0x99};
            uint8_t res[8] = {0};
            size_t res_len = 0;
            uds_handler.processRequest(req, 1, res, sizeof(res), res_len);
            last_uds_req = "0x99 (Unknown Service)";
            last_uds_res = "0x7F 0x99 0x11 (NRC: ServiceNotSupported)";
        }

        // Execute Periodic ECU Tasks (50 Hz rate)
        dynamics.setThrottle(static_cast<float>(throttle_level));
        dynamics.setBrake(static_cast<float>(brake_level));
        telemetry_task.runStep();
        dispatch_task.runStep();

        // Refresh Terminal Dashboard at ~20 Hz
        if (loop_count % 2 == 0) {
            std::cout << "\033[H"; // Move cursor to top-left
            std::cout << "\033[1;36m=======================================================================\033[0m\n";
            std::cout << "\033[1;37m        AUTOMOTIVE ECU GATEWAY - LIVE INTERACTIVE DASHBOARD            \033[0m\n";
            std::cout << "\033[1;36m=======================================================================\033[0m\n\n";

            // Meters
            drawBar("RPM", dynamics.getRpm(), 7000.0, 32, "\033[1;32m");
            std::cout << " rpm\n";
            drawBar("SPEED", dynamics.getSpeedKmH(), 240.0, 32, "\033[1;34m");
            std::cout << " km/h\n\n";

            // Telemetry Box
            std::cout << "  \033[1m[VEHICLE STATE]\033[0m\n";
            std::cout << "  Coolant Temp : " << std::fixed << std::setprecision(1) << dynamics.getCoolantTemp() << " C   |  ";
            std::cout << "Battery Voltage : " << std::fixed << std::setprecision(2) << dynamics.getBatteryVoltage() << " V\n";
            std::cout << "  Throttle Ped : " << throttle_level << " %          |  ";
            std::cout << "Brake Pedal     : " << brake_level << " %\n\n";

            // Bus Traffic
            std::cout << "  \033[1m[CAN BUS METRICS (ISO 11898)]\033[0m\n";
            std::cout << "  Total CAN Frames Transmitted : " << transceiver.getTransmittedCount() << "\n";
            std::cout << "  Active Telemetry Frame ID    : 0x100 (Cyclic 50 Hz)\n\n";

            // UDS Diag Console
            std::cout << "  \033[1m[ISO 14229 UDS DIAGNOSTIC STACK]\033[0m\n";
            std::cout << "  Last Request  : " << last_uds_req << "\n";
            std::cout << "  Last Response : \033[1;33m" << last_uds_res << "\033[0m\n\n";

            // Control Guide
            std::cout << "\033[1;30;47m  CONTROLS: [W] Gas  [S] Brake  [A] Coast  [1..4] Diag Query  [Q] Exit \033[0m\n";
            std::cout.flush();
        }

        loop_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // Restore terminal & Show cursor
    std::cout << "\033[?25h\033[2J\033[H";
    std::cout << "ECU Simulation terminated gracefully. Total TX frames: " 
              << transceiver.getTransmittedCount() << "\n";

    return 0;
}