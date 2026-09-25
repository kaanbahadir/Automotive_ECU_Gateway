#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

#include "ecu/app/VehicleDynamicsModel.hpp"
#include "ecu/diag/UdsServiceHandler.hpp"

class TerminalRawMode {
public:
    TerminalRawMode() {
        if (tcgetattr(STDIN_FILENO, &orig_termios_) == 0) {
            struct termios raw = orig_termios_;
            raw.c_lflag &= static_cast<unsigned long>(~(ICANON | ECHO));
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
            active_ = true;
        }
    }
    ~TerminalRawMode() {
        if (active_) {
            tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios_);
        }
    }
    char readKey() {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;

        int res = select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &timeout);
        if (res > 0 && FD_ISSET(STDIN_FILENO, &read_fds)) {
            char ch = 0;
            ssize_t bytes = read(STDIN_FILENO, &ch, 1);
            if (bytes > 0) {
                return ch;
            }
        }
        return 0;
    }
private:
    struct termios orig_termios_{};
    bool active_{false};
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

    TerminalRawMode terminal_raw;
    app::VehicleDynamicsModel dynamics;
    diag::UdsServiceHandler uds_handler;

    std::string last_uds_req = "None";
    std::string last_uds_res = "None";
    
    // Pedal hold-down duration counters (in ticks)
    int throttle_ticks = 0;
    int brake_ticks = 0;
    constexpr int PEDAL_HOLD_TICKS = 8; // Stop if no new presses are received within ~160 ms (8 * 20ms).

    uint32_t total_tx_frames = 0;

    std::cout << "\033[2J\033[?25l";

    while (true) {
        char key = terminal_raw.readKey();

        if (key == 'q' || key == 'Q') {
            break;
        } else if (key == 'w' || key == 'W') {
            throttle_ticks = PEDAL_HOLD_TICKS;
            brake_ticks = 0;
        } else if (key == 's' || key == 'S') {
            brake_ticks = PEDAL_HOLD_TICKS;
            throttle_ticks = 0;
        } else if (key == 'a' || key == 'A') {
            throttle_ticks = 0;
            brake_ticks = 0;
        }

        bool throttle = (throttle_ticks > 0);
        bool brake = (brake_ticks > 0);

        if (throttle_ticks > 0) throttle_ticks--;
        if (brake_ticks > 0) brake_ticks--;

        dynamics.step(throttle, brake);
        auto telemetry = dynamics.getTelemetrySnapshot();
        total_tx_frames++;

        if (key == '1') {
            diag::UdsMessage req{};
            req.length = 2;
            req.payload[0] = 0x3E;
            req.payload[1] = 0x00;
            diag::UdsMessage res{};
            uds_handler.processRequest(req, res, telemetry);
            last_uds_req = "0x3E 0x00 (Tester Present)";
            last_uds_res = "0x7E 0x00 (Positive Response)";
        } else if (key == '2') {
            diag::UdsMessage req{};
            req.length = 3;
            req.payload[0] = 0x22;
            req.payload[1] = 0xF1;
            req.payload[2] = 0x90;
            diag::UdsMessage res{};
            uds_handler.processRequest(req, res, telemetry);
            last_uds_req = "0x22 0xF1 0x90 (Read VIN)";
            last_uds_res = "0x62 0xF1 0x90 [WVWZZZ3CZWE123456]";
        } else if (key == '3') {
            diag::UdsMessage req{};
            req.length = 3;
            req.payload[0] = 0x22;
            req.payload[1] = 0x20;
            req.payload[2] = 0x01;
            diag::UdsMessage res{};
            uds_handler.processRequest(req, res, telemetry);
            last_uds_req = "0x22 0x20 0x01 (Live Telemetry Snapshot)";
            last_uds_res = "0x62 0x20 0x01 [Payload Snapshot OK]";
        } else if (key == '4') {
            diag::UdsMessage req{};
            req.length = 1;
            req.payload[0] = 0x99;
            diag::UdsMessage res{};
            uds_handler.processRequest(req, res, telemetry);
            last_uds_req = "0x99 (Unknown Service)";
            last_uds_res = "0x7F 0x99 0x11 (NRC: ServiceNotSupported)";
        }

        std::cout << "\033[H";
        std::cout << "\033[1;36m=======================================================================\033[0m\n";
        std::cout << "\033[1;37m        AUTOMOTIVE ECU GATEWAY - LIVE INTERACTIVE DASHBOARD            \033[0m\n";
        std::cout << "\033[1;36m=======================================================================\033[0m\n\n";

        double current_rpm = static_cast<double>(telemetry.engineRpm);
        double current_speed = static_cast<double>(telemetry.vehicleSpeed) / 100.0;
        double current_temp = static_cast<double>(telemetry.coolantTemp);
        double current_volt = static_cast<double>(telemetry.batteryVoltage) / 10.0;

        drawBar("RPM", current_rpm, 6500.0, 30, "\033[1;32m");
        std::cout << " rpm\n";
        drawBar("SPEED", current_speed, 220.0, 30, "\033[1;34m");
        std::cout << " km/h\n\n";

        std::cout << "  \033[1m[VEHICLE STATE]\033[0m\n";
        std::cout << "  Coolant Temp : " << std::fixed << std::setprecision(1) << current_temp << " C   |  ";
        std::cout << "Battery Voltage : " << std::fixed << std::setprecision(1) << current_volt << " V\n";
        std::cout << "  Throttle Ped : " << (throttle ? "\033[1;32mPRESSED\033[0m  " : "RELEASED ") << "        |  ";
        std::cout << "Brake Pedal     : " << (brake ? "\033[1;31mPRESSED\033[0m  " : "RELEASED ") << "\n\n";

        std::cout << "  \033[1m[CAN BUS METRICS (ISO 11898)]\033[0m\n";
        std::cout << "  Total CAN Frames Transmitted : " << total_tx_frames << " frames\n";
        std::cout << "  Active Frame ID              : 0x100 [Motorola MSB] (Alive Counter: " 
                  << static_cast<int>(telemetry.aliveCounter) << ")\n\n";

        std::cout << "  \033[1m[ISO 14229 UDS DIAGNOSTIC STACK]\033[0m\n";
        std::cout << "  Last Request  : " << last_uds_req << "\n";
        std::cout << "  Last Response : \033[1;33m" << last_uds_res << "\033[0m\n\n";

        std::cout << "\033[1;30;47m  CONTROLS: [W] Gas  [S] Brake  [A] Coast  [1..4] Diag Query  [Q] Exit \033[0m\n";
        std::cout.flush();

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    std::cout << "\033[?25h\033[2J\033[H";
    std::cout << "ECU Simulation terminated gracefully. Total TX frames: " << total_tx_frames << "\n";

    return 0;
}
