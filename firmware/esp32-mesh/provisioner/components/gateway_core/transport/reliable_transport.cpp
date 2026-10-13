#include "reliable_transport.hpp"
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "opcode.hpp"
#include <utility>
#include "esp_log.h"

static constexpr EventBits_t TX_EXITED = (1 << 0);
static constexpr EventBits_t RX_EXITED = (1 << 1);
static constexpr EventBits_t DISPATCH_EXITED = (1 << 2);


ReliableTransport::ReliableTransport(std::shared_ptr<ISerialPort> port) : port(port), parser(crc), ack_builder(crc), arq(port, ack_builder) 
{
    tx_mutex = xSemaphoreCreateMutex();
    dispatch_mutex = xSemaphoreCreateMutex();
    exit_events = xEventGroupCreate();

    configASSERT(tx_mutex != nullptr);
    configASSERT(dispatch_mutex != nullptr);
    configASSERT(exit_events != nullptr);
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
    
    if (exit_events) {
        vEventGroupDelete(exit_events);
    }

}

communication_status_t ReliableTransport::start() 
{
    if (is_running.load()) {
        return COMMUNICATION_SUCCESS;   
    }

    auto st = port->open();
    if (st != COMMUNICATION_SUCCESS) {
        return st;
    }

    xEventGroupClearBits(exit_events, TX_EXITED | RX_EXITED | DISPATCH_EXITED);
    expected_exit_bits = 0;
    is_running = true;

    if (xTaskCreate(tx_task_func, "tx_task", 4096, this, 1, &tx_task) == pdPASS) {
        expected_exit_bits |= TX_EXITED;
    }
    if (xTaskCreate(rx_task_func, "rx_task", 4096, this, 1, nullptr) == pdPASS) {
        expected_exit_bits |= RX_EXITED;
    }
    if (xTaskCreate(dispatch_task_func, "dispatch_task", 4096, this, 1, &dispatch_task) == pdPASS) {
        expected_exit_bits |= DISPATCH_EXITED;
    }

    if (expected_exit_bits != (TX_EXITED | RX_EXITED | DISPATCH_EXITED)) {
        ESP_LOGE("ReliableTransport", "failed to create tasks");
        stop();                                  
        return COMMUNICATION_FAILED;
    }

    return COMMUNICATION_SUCCESS;
}

communication_status_t ReliableTransport::stop() {
    if (!is_running.exchange(false)) {
        return COMMUNICATION_SUCCESS;
    }

    if (tx_task) {
        xTaskNotifyGive(tx_task);
    }

    if (dispatch_task) {
        xTaskNotifyGive(dispatch_task);
    }

    if (expected_exit_bits != 0) {            
        EventBits_t bits = xEventGroupWaitBits(exit_events, expected_exit_bits, pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));
        if ((bits & expected_exit_bits) != expected_exit_bits) {
            ESP_LOGE("ReliableTransport", "Timeout waiting for tasks to exit");
            return COMMUNICATION_ERROR;
        }
    }

    port->close();


    tx_task = nullptr;
    dispatch_task = nullptr;
    expected_exit_bits = 0;

    return COMMUNICATION_SUCCESS;
}

bool ReliableTransport::enqueue_frame(const std::vector<uint8_t>& raw_frame, bool reliable) {
    if (!is_running.load() || tx_task == nullptr) {
        return false;
    }   

    uint8_t seq = (raw_frame.size() > FRAME_OFF_SEQ) ? raw_frame[FRAME_OFF_SEQ] : 0;
    xSemaphoreTake(tx_mutex, portMAX_DELAY);
    struct TxItem item = {
        .frame = raw_frame,
        .reliable = reliable,
        .seq = seq
    };
    tx_queue.push(std::move(item));
    xSemaphoreGive(tx_mutex);

    xTaskNotifyGive(tx_task);

    return true;
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
            dispatch_queue.push(std::move(f));
            xSemaphoreGive(dispatch_mutex);

            xTaskNotifyGive(dispatch_task);
        }
    }
}

void ReliableTransport::dispatch_loop() {
    while (is_running.load()) {
        
        BaseType_t notified = xTaskNotifyWait(0, ULONG_MAX, nullptr, portMAX_DELAY);
        if (notified != pdTRUE || !is_running.load()) {
            break;
        }

        while (is_running.load()) {
            bool has_frame = false;
            RawFrame frame;

            xSemaphoreTake(dispatch_mutex, portMAX_DELAY);
            if (!dispatch_queue.empty()) {
                frame = std::move(dispatch_queue.front());
                dispatch_queue.pop();
                has_frame = true;
            }
            xSemaphoreGive(dispatch_mutex);

            if (!has_frame) {
                break; 
            }

            if (on_frame_cb) {
                on_frame_cb(frame);
                OpCode current_opcode = static_cast<OpCode>(frame.opcode);
                if (current_opcode == OpCode::CMD_GROUP_ADD ||
                    current_opcode == OpCode::CMD_GROUP_DELETE ||
                    current_opcode == OpCode::CMD_MODEL_PUB_SET) {
                    vTaskDelay(pdMS_TO_TICKS(2000));
                }

            }
    
        }

    }
}

void ReliableTransport::tx_loop() 
{ 
    while (is_running.load()) {

        if (xTaskNotifyWait(0, ULONG_MAX, nullptr, portMAX_DELAY) != pdTRUE) {
            break;
        }

        while(is_running.load()) {
            TxItem item;
            bool has_item = false;

            xSemaphoreTake(tx_mutex, portMAX_DELAY);
            if (!tx_queue.empty()) {
                item = std::move(tx_queue.front());
                tx_queue.pop();
                has_item = true;
            }
            xSemaphoreGive(tx_mutex);

            if (!has_item) {
                break;
            }
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
    xEventGroupSetBits(self->exit_events, TX_EXITED);
    vTaskDelete(nullptr);
}


void ReliableTransport::rx_task_func(void* arg)
{
    auto* self = static_cast<ReliableTransport*>(arg);

    self->rx_loop();
    xEventGroupSetBits(self->exit_events, RX_EXITED);
    vTaskDelete(nullptr);
}

void ReliableTransport::dispatch_task_func(void* arg)
{
    auto* self = static_cast<ReliableTransport*>(arg);

    self->dispatch_loop();
    xEventGroupSetBits(self->exit_events, DISPATCH_EXITED);
    vTaskDelete(nullptr);
}
