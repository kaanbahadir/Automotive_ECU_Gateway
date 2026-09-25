#include <cassert>
#include <iostream>
#include "ecu/core/CanRingBuffer.hpp"
#include "ecu/proto/CanFrame.hpp"

int main() {
    std::cout << "[RUNNING] CAN Ring Buffer Unit Test..." << std::endl;

    constexpr std::size_t BUFFER_SIZE = 3;
    ecu::core::CanRingBuffer<BUFFER_SIZE> ringBuffer;

    // 1. Initial State Check
    assert(ringBuffer.isEmpty());
    assert(!ringBuffer.isFull());
    assert(ringBuffer.size() == 0);
    assert(ringBuffer.capacity() == BUFFER_SIZE);

    // 2. Empty Queue Pull Protection (Underflow)
    ecu::proto::CanFrame dummyFrame{};
    assert(!ringBuffer.pop(dummyFrame));

    // 3. Filling the Tail to Capacity
    ecu::proto::CanFrame frame1{};
    frame1.id = 0x101;
    frame1.data[0] = 0xAA;

    ecu::proto::CanFrame frame2{};
    frame2.id = 0x102;
    frame2.data[0] = 0xBB;

    ecu::proto::CanFrame frame3{};
    frame3.id = 0x103;
    frame3.data[0] = 0xCC;

    assert(ringBuffer.push(frame1));
    assert(ringBuffer.push(frame2));
    assert(ringBuffer.push(frame3));

    assert(ringBuffer.isFull());
    assert(ringBuffer.size() == 3);

    // 4. Overflow Protection (Must reject new elements when full)
    ecu::proto::CanFrame overflowFrame{};
    overflowFrame.id = 0x999;
    assert(!ringBuffer.push(overflowFrame));

    // 5. FIFO Sequence Verification (First-In, First-Out)
    ecu::proto::CanFrame poppedFrame{};

    assert(ringBuffer.pop(poppedFrame));
    assert(poppedFrame.id == 0x101);
    assert(poppedFrame.data[0] == 0xAA);

    assert(ringBuffer.pop(poppedFrame));
    assert(poppedFrame.id == 0x102);

    assert(ringBuffer.pop(poppedFrame));
    assert(poppedFrame.id == 0x103);

    // 6. The queue must be empty again.
    assert(ringBuffer.isEmpty());
    assert(!ringBuffer.pop(poppedFrame));

    // 7. Wrap-around Control
    ecu::proto::CanFrame wrapFrame{};
    wrapFrame.id = 0x200;
    assert(ringBuffer.push(wrapFrame));
    assert(ringBuffer.size() == 1);
    assert(ringBuffer.pop(poppedFrame));
    assert(poppedFrame.id == 0x200);

    std::cout << "[PASSED] All ring buffer assertions passed successfully!" << std::endl;
    return 0;
}