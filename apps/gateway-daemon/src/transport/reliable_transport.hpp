#ifndef RELIABLE_TRANSPORT_HPP
#define RELIABLE_TRANSPORT_HPP

#include "iserial_port.hpp"
#include "frame_praser.hpp"
#include "ack_builder.hpp"
#include "arq_controller.hpp"
#include "crc_calculator.hpp"
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
/**
 * @brief 
 * 
 */
using frame_cb_t = std::function<void(const RawFrame&)>;


class ReliableTransport {
public:
    explicit ReliableTransport(std::shared_ptr<ISerialPort> port);
    ~ReliableTransport();

    communication_status_t start();
    communication_status_t stop();

    void enqueue_frame(const std::vector<uint8_t>& raw_frame, bool reliable = true);
    void set_frame_callback(frame_cb_t cb) { 
        on_frame_cb = std::move(cb); 
    }

private:
    std::shared_ptr<ISerialPort> port;
    CrcCalculator crc;
    FrameParser parser;
    AckBuilder ack_builder;
    ArqController arq;

    struct TxItem {
        std::vector<uint8_t> frame;
        bool reliable;
        uint8_t seq;
    };

    std::queue<TxItem> tx_queue;
    std::mutex queue_mutex;
    std::condition_variable tx_cv;

    std::thread rx_thread;
    std::thread tx_thread;
    std::thread dispatch_thread;

    std::atomic<bool> is_running { false };
    frame_cb_t on_frame_cb;

    std::queue<RawFrame>  dispatch_queue;
    std::mutex dispatch_mutex;
    std::condition_variable dispatch_cv;

    void rx_loop();
    void tx_loop();
    void dispatch_loop();
};

#endif 