#pragma once

#include <array>
#include <cstddef>
#include "ecu/proto/CanFrame.hpp"

namespace ecu::core {
    template <std::size_t Capacity>
    class CanRingBuffer {
        static_assert(Capacity > 0, "Capacity must be greater than 0");

    public:
        CanRingBuffer() : head_(0), tail_(0), count_(0) {}

        //Adds a new CAN frame to the queue (Producer Task)
        bool push(const proto::CanFrame& frame) noexcept {
            if (isFull()) {
                return false; //Buffer full, data could not be written (buffer overflow)
            }
            
            buffer_[head_] = frame;
            head_ = (head_ + 1) % Capacity;
            ++count_;
            return true;
        }

        //Retrieves the oldest CAN frame from the queue (Consumer Task)
        bool pop(proto::CanFrame& outFrame) noexcept {
            if (isEmpty()) {
                return false; //Buffer empty, no data to read
            }

            outFrame = buffer_[tail_];
            tail_ = (tail_ + 1) % Capacity;
            --count_;
            return true;
        }

        [[nodiscard]] bool isFull() const noexcept {
            return count_ == Capacity;
        }

        [[nodiscard]] bool isEmpty() const noexcept {
            return count_ == 0;
        }

        [[nodiscard]] std::size_t size() const noexcept {
            return count_;
        }

        [[nodiscard]] constexpr std::size_t capacity() const noexcept {
            return Capacity;
        }

        void clear() noexcept {
            head_ = 0;
            tail_ = 0;
            count_ = 0;
        }

    private:
        std::array<proto::CanFrame, Capacity> buffer_{};
        std::size_t head_{0};   //The index at which the new element will be written
        std::size_t tail_{0};   //Index of the next element to be read
        std::size_t count_{0};  //Current number of staff
    };
} // namespace ecu::core