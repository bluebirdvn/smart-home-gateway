#include "arq_controller.hpp"
#include <iostream>
#include <chrono>
#include <cinttypes>
ArqController::ArqController(std::shared_ptr<ISerialPort> port, const AckBuilder& ack_builder) : port(port), ack_builder(ack_builder) {}

communication_status_t ArqController::send_reliable(const std::vector<uint8_t>& frame, uint8_t seq)
{
    for (uint8_t attempt = 0; attempt < ARQ_MAX_RETRY; attempt++) {
        port->write_bytes(frame);

        std::unique_lock<std::mutex> lk(ack_mutex);

        last_nacked = -1;

        bool ok = ack_cv.wait_for(lk, std::chrono::milliseconds(ARQ_TIMEOUT_MS),[this, seq] {
                return last_acked == (int)seq || last_nacked == (int)seq;
            });

        if (ok && last_acked == (int)seq) {
            printf("Received opcode: %u\n", static_cast<unsigned int>(frame[0]));
            fflush(stdout);
            return COMMUNICATION_SUCCESS;
        }
    }

    std::cerr << "Failed after " << (int)ARQ_MAX_RETRY << " attempts, seq=" << (int)seq << "\n";
    return COMMUNICATION_FAILED;
}

void ArqController::notify_ack(uint8_t seq) {
    {
        std::lock_guard<std::mutex> lk(ack_mutex);
        last_acked = (int)seq;
    }
    ack_cv.notify_all();
}

void ArqController::notify_nack(uint8_t seq) {
    {
        std::lock_guard<std::mutex> lk(ack_mutex);
        last_nacked = (int)seq;
    }
    ack_cv.notify_all();
}