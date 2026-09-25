cmake_minimum_required(VERSION 3.16)
project(AutomotiveEcuGateway
    VERSION 1.0.0
    DESCRIPTION "Automotive ECU Telemetry and UDS Gateway Simulation"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Strict automotive compiler flags
add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow)

# Include directories
include_directories(include)

# Main Simulation Application
add_executable(ecu_gateway_app src/main.cpp)

# Test Infrastructure (CTest)
enable_testing()

add_executable(test_codec tests/unit/test_codec.cpp)
add_test(NAME CAN_Codec_Test COMMAND test_codec)

add_executable(test_ring_buffer tests/unit/test_ring_buffer.cpp)
add_test(NAME LockFree_RingBuffer_Test COMMAND test_ring_buffer)

add_executable(test_tasks tests/unit/test_tasks.cpp)
add_test(NAME Task_Scheduler_Test COMMAND test_tasks)

add_executable(test_uds tests/unit/test_uds.cpp)
add_test(NAME UDS_Diagnostics_Test COMMAND test_uds)

add_executable(test_hal tests/unit/test_hal.cpp)
add_test(NAME HAL_Transceiver_Test COMMAND test_hal)

add_executable(test_app tests/unit/test_app.cpp)
add_test(NAME Vehicle_Dynamics_App_Test COMMAND test_app)

add_executable(test_os tests/unit/test_os.cpp)
add_test(NAME OSAL_Timer_Test COMMAND test_os)