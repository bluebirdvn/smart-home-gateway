#include "reliable_transport.hpp"
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
ReliableTransport::ReliableTransport(std::shared_ptr<ISerialPort> port) : port(port), parser(crc), ack_builder(crc), arq(port, ack_builder) 
{
    tx_mutex = xSemaphoreCreateMutex();
    dispatch_mutex = xSemaphoreCreateMutex();

    configASSERT(tx_mutex != nullptr);
    configASSERT(dispatch_mutex != nullptr);
}

ReliableTransport::~ReliableTransport() 
{ 
    stop(); 

    if (tx_mutex) {
        vSemaphoreDelete(tx_mutex);
    }
    if (dispatch_mutex) {
        vSemaphoreDelete(dispatch_mutex);
    }
    

}

communication_status_t ReliableTransport::start() {
    auto st = port->open();
    if (st != COMMUNICATION_SUCCESS) {
        return st;
    }

    is_running = true;

    BaseType_t tx = xTaskCreate(tx_task_func, "tx_task", 4096, this, 1, &tx_task);
    BaseType_t rx = xTaskCreate(rx_task_func, "rx_task", 4096, this, 1, nullptr);
    BaseType_t dispatch = xTaskCreate(dispatch_task_func, "dispatch_task", 4096, this, 1, &dispatch_task);
    
    return COMMUNICATION_SUCCESS;
}

communication_status_t ReliableTransport::stop() {
    if (!is_running.exchange(false)) {
        return COMMUNICATION_SUCCESS;
    }


    port->close();
    if (tx_task) {
        xTaskNotifyGive(tx_task);
    }

    if (dispatch_task) {
        xTaskNotifyGive(dispatch_task);
    }
    return COMMUNICATION_SUCCESS;
}

void ReliableTransport::enqueue_frame(const std::vector<uint8_t>& raw_frame, bool reliable) {
    uint8_t seq = (raw_frame.size() > FRAME_OFF_SEQ) ? raw_frame[FRAME_OFF_SEQ] : 0;
    xSemaphoreTake(tx_mutex, portMAX_DELAY);
    struct TxItem item = {
        .frame = raw_frame,
        .reliable = reliable,
        .seq = seq
    };
    tx_queue.push(item);
    xSemaphoreGive(tx_mutex);

    xTaskNotifyGive(tx_task);
}

void ReliableTransport::rx_loop() {
    uint8_t byte = 0;
    while (is_running.load()) {
        if (port->read_byte(byte) <= 0) {
            continue;
        }

        auto result = parser.feed(byte);
        if (!result) {
            continue;
        }

        RawFrame& f = *result;

        if (ack_builder.is_ack(f)) {
            arq.notify_ack(f.seq);

        } else if (ack_builder.is_nack(f)) {
            arq.notify_nack(f.seq);

        } else {
            auto ack = ack_builder.build_ack(f.seq);
            port->write_bytes(ack);

            xSemaphoreTake(dispatch_mutex, portMAX_DELAY);
            dispatch_queue.push(f);
            xSemaphoreGive(dispatch_mutex);

            xTaskNotifyGive(dispatch_task);
        }
    }
}

void ReliableTransport::dispatch_loop() {
    while (is_running.load()) {
        bool has_frame = false;

        RawFrame frame;
        {
            BaseType_t notified = xTaskNotifyWait(0, ULONG_MAX, nullptr, portMAX_DELAY);
            if (notified != pdTRUE) {
                break;
            }

            if (!is_running.load()) {
                break;
            }
            xSemaphoreTake(dispatch_mutex, portMAX_DELAY);

            if (!dispatch_queue.empty()) {
                frame = dispatch_queue.front();
                dispatch_queue.pop();
                has_frame = true;
            }
            xSemaphoreGive(dispatch_mutex);

        }
        if (on_frame_cb && has_frame) {
            on_frame_cb(frame);
        }
    }
}

void ReliableTransport::tx_loop() { 
    while (is_running.load()) {
        TxItem item;
        bool has_item = false;
        {
            BaseType_t notified = xTaskNotifyWait(0, ULONG_MAX, nullptr, portMAX_DELAY);

            if (notified != pdTRUE) {
                break;
            }

            if (!is_running.load()) {
                break;
            }
            xSemaphoreTake(tx_mutex, portMAX_DELAY);

            if (!tx_queue.empty()) {
                item = tx_queue.front();
                tx_queue.pop();
                has_item = true;
            }

            xSemaphoreGive(tx_mutex);
        }

        if (has_item) {
            if (item.reliable) {
                arq.send_reliable(item.frame, item.seq);
            } else {
                port->write_bytes(item.frame);
            }
        }
    }
}

void ReliableTransport::tx_task_func(void* arg)
{
    auto* self = static_cast<ReliableTransport*>(arg);

    self->tx_loop();

    vTaskDelete(nullptr);
}


void ReliableTransport::rx_task_func(void* arg)
{
    auto* self = static_cast<ReliableTransport*>(arg);

    self->rx_loop();

    vTaskDelete(nullptr);
}

void ReliableTransport::dispatch_task_func(void* arg)
{
    auto* self = static_cast<ReliableTransport*>(arg);

    self->dispatch_loop();

    vTaskDelete(nullptr);
}
