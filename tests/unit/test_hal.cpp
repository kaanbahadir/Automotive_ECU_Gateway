#include <cassert>
#include <iostream>
#include "ecu/core/CanRingBuffer.hpp"
#include "ecu/hal/VirtualCanTransceiver.hpp"
#include "ecu/tasks/CanDispatchTask.hpp"

int main() {
    std::cout << "[RUNNING] HAL & Transceiver Dispatch Integration Test..." << std::endl;

    constexpr std::size_t QUEUE_CAPACITY = 4;
    ecu::core::CanRingBuffer<QUEUE_CAPACITY> queue;
    ecu::hal::VirtualCanTransceiver<8> transceiver;
    ecu::tasks::CanDispatchTask<QUEUE_CAPACITY> dispatcher(queue, transceiver);

    // 1. Test: No transmission should occur when the queue is empty.
    assert(!dispatcher.step());
    assert(dispatcher.getTransmittedFramesCount() == 0);
    assert(transceiver.getTotalTransmittedCount() == 0);

    // 2. Test: Add the frame to the queue and verify that it was transmitted through the transceiver.
    ecu::proto::CanFrame frame1{};
    frame1.id = 0x100;
    frame1.dlc = 8;
    frame1.data = {1, 2, 3, 4, 5, 6, 7, 8};
    assert(queue.push(frame1));

    assert(dispatcher.step());
    assert(dispatcher.getTransmittedFramesCount() == 1);
    assert(transceiver.getTotalTransmittedCount() == 1);
    assert(transceiver.getLastTransmittedFrame().id == 0x100);
    assert(transceiver.getLastTransmittedFrame().data[0] == 1);

    // 3. Test: Loopback RX test
    ecu::proto::CanFrame rxFrameToInject{};
    rxFrameToInject.id = 0x200;
    rxFrameToInject.dlc = 4;
    rxFrameToInject.data = {0xAA, 0xBB, 0xCC, 0xDD, 0, 0, 0, 0};
    transceiver.injectRxFrame(rxFrameToInject);

    ecu::proto::CanFrame receivedFrame{};
    assert(transceiver.receive(receivedFrame));
    assert(receivedFrame.id == 0x200);
    assert(receivedFrame.dlc == 4);
    assert(receivedFrame.data[1] == 0xBB);

    // It should return empty when a re-read is attempted.
    assert(!transceiver.receive(receivedFrame));

    std::cout << "[PASSED] All HAL & Transceiver assertions passed successfully!" << std::endl;
    return 0;
}