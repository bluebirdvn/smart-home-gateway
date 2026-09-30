#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "mqtt_connection_config.h"
#include "mqtt_message.h"
#include <MQTTAsync.h>

#include <string>
#include <functional>
#include <queue>
#include <set>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <cstdint>

using MessageHandler = std::function<void(const MQTTMessage&)>;
using ReasonCallback = std::function<void(int reason)>;
using Callback = std::function<void()>;

class MQTTClient {
public:
    explicit MQTTClient(const MQTTConnectionConfig& config);
    ~MQTTClient();

    MQTTClient(const MQTTClient&) = delete;
    MQTTClient& operator=(const MQTTClient&) = delete;

    bool connect();
    bool disconnect();
    bool subscribe(const std::string& topic, MQTTQoS qos = QOS_1_AT_LEAST_ONCE);
    bool unsubscribe(const std::string& topic);
    bool publish(const MQTTMessage& message);

    void setMessageHandler(MessageHandler handler);
    void setConnectHandler(Callback callback);
    void setDisconnectHandler(ReasonCallback callback);

    static void connlost(void* context, char* cause);
    static int msgarrvd(void* context, char* topicName, int topicLen, MQTTAsync_message* message);
    static void onConnect(void* context, MQTTAsync_successData* response);
    static void onConnectFailure(void* context, MQTTAsync_failureData* response);
    static void onDisconnect(void* context, MQTTAsync_successData* response);
    static void onDisconnectFailure(void* context, MQTTAsync_failureData* response);
    static void onSubscribe(void* context, MQTTAsync_successData* response);
    static void onSubscribeFailure(void* context, MQTTAsync_failureData* response);
    static void onPublish(void* context, MQTTAsync_successData* response);
    static void onPublishFailure(void* context, MQTTAsync_failureData* response);

private:
    void resubscribeAll();
    void reconnectWithBackoff();
    void ProcessMessage();
    void enqueueMessage(const std::string& topic, const std::string& payload, int qos, bool retain);

    MQTTConnectionConfig mqtt_config;
    MQTTAsync mqtt_handle;
    std::atomic<bool> mqtt_client_connected;
    std::atomic<bool> mqtt_client_stop;
    std::atomic<bool> is_reconnecting{false};
    uint32_t reconnect_delay;

    std::set<std::string> subscribed_topics;
    std::mutex lock_topic;

    std::queue<MQTTMessage> message_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_condvar;

    std::thread mqtt_handle_thread;

    MessageHandler message_handler;
    Callback connect_handler;
    ReasonCallback disconnect_handler;
};

#endif 