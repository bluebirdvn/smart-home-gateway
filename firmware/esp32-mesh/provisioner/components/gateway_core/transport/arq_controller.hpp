#ifndef ARQ_CONTROLLER_HPP
#define ARQ_CONTROLLER_HPP

#include "iserial_port.hpp"
#include "ack_builder.hpp"
#include <mutex>
#include <condition_variable>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#define ARQ_MAX_RETRY   3
#define ARQ_TIMEOUT_MS  500

class ArqController {
public:
    ArqController(std::shared_ptr<ISerialPort> port, const AckBuilder& ack_builder);
    ~ArqController();
    communication_status_t send_reliable(const std::vector<uint8_t>& frame, uint8_t seq);

    void notify_ack (uint8_t seq);
    void notify_nack(uint8_t seq);

private:
    std::shared_ptr<ISerialPort> port;
    const AckBuilder& ack_builder;
    SemaphoreHandle_t sem_ack;
    TaskHandle_t waiting_task {nullptr};
    int last_acked { -1 };
    int last_nacked { -1 };
};

#endif
