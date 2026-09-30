#ifndef ARQ_CONTROLLER_HPP
#define ARQ_CONTROLLER_HPP

#include "iserial_port.hpp"
#include "ack_builder.hpp"
#include <mutex>
#include <condition_variable>
#include <memory>

#define ARQ_MAX_RETRY   3
#define ARQ_TIMEOUT_MS  500

/**
 * @brief manage reliable transmit data over serial port
 * this controller ensures that a frame successfully received by waiting ack
 * if nack is received or timeuot occurs, it automatically retransmit packet with ARQ_MAX_RETRY
 */
class ArqController {
public:
    /**
     * @brief Construct a new Arq Controller object
     * 
     * @param port pointer to serial port
     * @param ack_builder reference to ackBuilder object
     */
    ArqController(std::shared_ptr<ISerialPort> port, const AckBuilder& ack_builder);

    /**
     * @brief send data reliable and block until receiving ack message
     * 
     * @param frame frame after encode to raw bytes
     * @param seq sequence number of packet 
     * @return communication_status_t COMMUNICATION_SUCCESS if success, COMMUNICATION_FAILED if max reties exceeded
     */
    communication_status_t send_reliable(const std::vector<uint8_t>& frame, uint8_t seq);

    void notify_ack (uint8_t seq);
    void notify_nack(uint8_t seq);

private:
    std::shared_ptr<ISerialPort> port;
    const AckBuilder& ack_builder;
    std::mutex ack_mutex;
    std::condition_variable ack_cv;
    int last_acked { -1 }; // sequence of last ack
    int last_nacked { -1 }; // sequence of last nack
};

#endif
