#include "arq_controller.hpp"
#include <iostream>
#include <chrono>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
ArqController::ArqController(std::shared_ptr<ISerialPort> port, const AckBuilder& ack_builder) : port(port), ack_builder(ack_builder) 
{
    sem_ack = xSemaphoreCreateMutex();
    configASSERT(sem_ack != nullptr);
}

ArqController::~ArqController() {
    if (sem_ack) {
        vSemaphoreDelete(sem_ack);
    }
}


communication_status_t ArqController::send_reliable(const std::vector<uint8_t>& frame, uint8_t seq)
{
    for (uint8_t attempt = 0; attempt < ARQ_MAX_RETRY; attempt++) {
        port->write_bytes(frame);

        xSemaphoreTake(sem_ack, portMAX_DELAY);

        last_nacked = -1;
        last_acked = -1;

        waiting_task = xTaskGetCurrentTaskHandle();
        xSemaphoreGive(sem_ack);
        xTaskNotifyStateClear(nullptr);
        bool got_ack = false;
        bool got_nack = false;

        TickType_t start_tick = xTaskGetTickCount();
        TickType_t timeout_ticks = pdMS_TO_TICKS(ARQ_TIMEOUT_MS);

        while(true) {
            TickType_t eslapsed_ticks = xTaskGetTickCount() - start_tick;
            if (eslapsed_ticks >= timeout_ticks) {
                break;
            }

            uint32_t notify_value = 0;
            BaseType_t notified = xTaskNotifyWait(0, ULONG_MAX, &notify_value, timeout_ticks - eslapsed_ticks);
            if (notified != pdTRUE) {
                break;
            }

            xSemaphoreTake(sem_ack, portMAX_DELAY);
            got_ack = (last_acked == seq);
            got_nack = (last_nacked == seq);
            xSemaphoreGive(sem_ack);

            if (got_ack || got_nack) {
                break;
            }


        }


        xSemaphoreTake(sem_ack, portMAX_DELAY);
        waiting_task = nullptr;
        xSemaphoreGive(sem_ack);

        if (got_ack) {
            return COMMUNICATION_SUCCESS;
        }
    }

    std::cerr << "[ARQ] Failed after " << (int)ARQ_MAX_RETRY << " attempts, seq=" << (int)seq << "\n";
    return COMMUNICATION_FAILED;
}

void ArqController::notify_ack(uint8_t seq) {
    TaskHandle_t task_to_notify = nullptr;

    xSemaphoreTake(sem_ack, portMAX_DELAY);
    last_acked = (int)seq;
    task_to_notify = waiting_task;
    xSemaphoreGive(sem_ack);

    if (task_to_notify != nullptr) {
        xTaskNotifyGive(task_to_notify);
    }
}

void ArqController::notify_nack(uint8_t seq) {
    TaskHandle_t task_to_notify = nullptr;

    xSemaphoreTake(sem_ack, portMAX_DELAY);
    last_nacked = (int)seq;
    task_to_notify = waiting_task;
    xSemaphoreGive(sem_ack);

    if (task_to_notify != nullptr) {
        xTaskNotifyGive(task_to_notify);
    }
}