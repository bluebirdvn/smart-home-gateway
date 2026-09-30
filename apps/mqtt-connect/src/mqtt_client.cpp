#include "mqtt_client.h"
#include <cstdio>
#include <chrono>
#include <thread>


MQTTClient::MQTTClient(const MQTTConnectionConfig& config)
    : mqtt_config(config)           
    , mqtt_handle(nullptr)
    , mqtt_client_connected(false)
    , mqtt_client_stop(false)
    , reconnect_delay(1000)
{
    int rc = MQTTAsync_create(
        &mqtt_handle,
        mqtt_config.brokerURL.c_str(),
        mqtt_config.clientID.c_str(),
        MQTTCLIENT_PERSISTENCE_NONE,
        nullptr);

    if (rc != MQTTASYNC_SUCCESS) {
        printf("MQTTAsync_create failed, rc=%d\n", rc);
        mqtt_handle = nullptr;
        return;
    }
    rc = MQTTAsync_setCallbacks(mqtt_handle, this, connlost, msgarrvd, nullptr);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("MQTTAsync_setCallbacks failed, rc=%d\n", rc);
    }

    mqtt_handle_thread = std::thread(&MQTTClient::ProcessMessage, this);
}

MQTTClient::~MQTTClient()
{
    mqtt_client_stop = true;
    while (is_reconnecting) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    queue_condvar.notify_all();
    if (mqtt_handle_thread.joinable()) {
        mqtt_handle_thread.join();
    }

    if (mqtt_client_connected) {
        disconnect();
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
    }
    if (mqtt_handle) {
        MQTTAsync_destroy(&mqtt_handle);
    }
}


bool MQTTClient::connect()
{
    if (!mqtt_handle) {
        return false;
    }
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.keepAliveInterval = mqtt_config.keepAliveInterval;
    conn_opts.cleansession      = mqtt_config.cleanSession ? 1 : 0;
    conn_opts.onSuccess         = MQTTClient::onConnect;
    conn_opts.onFailure         = MQTTClient::onConnectFailure;
    conn_opts.context           = this;

    if (!mqtt_config.userName.empty()) {
        conn_opts.username = mqtt_config.userName.c_str();
    }
    if (!mqtt_config.password.empty()) {
        conn_opts.password = mqtt_config.password.c_str();
    }

    MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;
    if (mqtt_config.havingTLS) {
        ssl_opts.trustStore = (!mqtt_config.tls.caFile.empty() && mqtt_config.tls.caFile != "0") ? mqtt_config.tls.caFile.c_str() : nullptr;
        ssl_opts.keyStore   = (!mqtt_config.tls.certFile.empty() && mqtt_config.tls.certFile != "0") ? mqtt_config.tls.certFile.c_str() : nullptr;
        ssl_opts.privateKey = (!mqtt_config.tls.keyFile.empty() && mqtt_config.tls.keyFile != "0") ? mqtt_config.tls.keyFile.c_str() : nullptr;

        ssl_opts.sslVersion = MQTT_SSL_VERSION_TLS_1_2;
        ssl_opts.enableServerCertAuth = mqtt_config.tls.verifyServer ? 1 : 0;
        conn_opts.ssl       = &ssl_opts;
    }

    MQTTAsync_willOptions will_opts = MQTTAsync_willOptions_initializer;
    if (!mqtt_config.lwt.willTopic.empty()) {
        will_opts.topicName = mqtt_config.lwt.willTopic.c_str();
        will_opts.message   = mqtt_config.lwt.willPayload.c_str();
        will_opts.qos       = static_cast<int>(mqtt_config.lwt.willQoS);
        will_opts.retained  = mqtt_config.lwt.willRetain ? 1 : 0;
        conn_opts.will      = &will_opts;
    }

    int rc = MQTTAsync_connect(mqtt_handle, &conn_opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("connect() failed, rc=%d\n", rc);
        return false;
    }
    return true;
}

bool MQTTClient::disconnect()
{
    if (!mqtt_handle) {
        return false;
    }

    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    disc_opts.onSuccess = MQTTClient::onDisconnect;
    disc_opts.onFailure = MQTTClient::onDisconnectFailure;
    disc_opts.context   = this;

    int rc = MQTTAsync_disconnect(mqtt_handle, &disc_opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("disconnect() failed, rc=%d\n", rc);
        return false;
    }
    return true;
}

bool MQTTClient::subscribe(const std::string& topic, MQTTQoS qos)
{
    if (!mqtt_handle || !mqtt_client_connected) {
        return false;
    }

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.onSuccess = MQTTClient::onSubscribe;
    opts.onFailure = MQTTClient::onSubscribeFailure;
    opts.context   = this;

    int rc = MQTTAsync_subscribe(mqtt_handle, topic.c_str(), static_cast<int>(qos), &opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("subscribe(%s) failed, rc=%d\n", topic.c_str(), rc);
        return false;
    }

    std::lock_guard<std::mutex> lock(lock_topic);
    subscribed_topics.insert(topic);
    return true;
}

bool MQTTClient::unsubscribe(const std::string& topic)
{
    if (!mqtt_handle || !mqtt_client_connected) {
        return false;
    }

    int rc = MQTTAsync_unsubscribe(mqtt_handle, topic.c_str(), nullptr);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("unsubscribe(%s) failed, rc=%d\n", topic.c_str(), rc);
        return false;
    }

    std::lock_guard<std::mutex> lock(lock_topic);
    subscribed_topics.erase(topic);
    return true;
}

bool MQTTClient::publish(const MQTTMessage& message)
{
    if (!mqtt_handle || !mqtt_client_connected) {
        return false;
    }

    MQTTAsync_message pubmsg  = MQTTAsync_message_initializer;
    pubmsg.payload    = const_cast<char*>(message.payload.c_str());
    pubmsg.payloadlen = static_cast<int>(message.payload.size());
    pubmsg.qos        = static_cast<int>(message.qos);
    pubmsg.retained   = message.retain ? 1 : 0;

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.onSuccess = MQTTClient::onPublish;
    opts.onFailure = MQTTClient::onPublishFailure;
    opts.context   = this;

    int rc = MQTTAsync_sendMessage(mqtt_handle, message.topic.c_str(), &pubmsg, &opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("publish(%s) failed, rc=%d\n", message.topic.c_str(), rc);
        return false;
    }
    return true;
}

void MQTTClient::setMessageHandler(MessageHandler handler)    
{ 
    message_handler  = std::move(handler); 
}


void MQTTClient::setConnectHandler(Callback callback)         
{ 
    connect_handler  = std::move(callback); 
}

void MQTTClient::setDisconnectHandler(ReasonCallback callback)
{ 
    disconnect_handler = std::move(callback); 
}


void MQTTClient::resubscribeAll()
{
    std::lock_guard<std::mutex> lock(lock_topic);
    for (const auto& topic : subscribed_topics) {
        MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
        opts.onSuccess = MQTTClient::onSubscribe;
        opts.onFailure = MQTTClient::onSubscribeFailure;
        opts.context   = this;
        int rc = MQTTAsync_subscribe(mqtt_handle, topic.c_str(), QOS_1_AT_LEAST_ONCE, &opts);
        if (rc != MQTTASYNC_SUCCESS) {
            printf("resubscribe(%s) failed, rc=%d\n", topic.c_str(), rc);
        }
    }
}

void MQTTClient::reconnectWithBackoff()
{
    if (mqtt_client_stop) {
        return;
    }

    const uint32_t MAX_DELAY_MS = 60000;
    printf("Reconnecting in %u ms...\n", reconnect_delay);

    is_reconnecting = true; 

    std::thread([this]() {
        uint32_t waited = 0;
        while(waited < reconnect_delay && !mqtt_client_stop) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            waited += 100;
        }

        if (!mqtt_client_stop) {
            reconnect_delay = std::min(reconnect_delay * 2, 30000U);
            connect();
        }
        
        is_reconnecting = false; 
    }).detach();
}

void MQTTClient::enqueueMessage(const std::string& topic, const std::string& payload, int qos, bool retain)
{
    MQTTMessage msg;
    msg.topic = topic;
    msg.payload = payload;
    msg.qos = static_cast<MQTTQoS>(qos);
    msg.retain = retain;

    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        message_queue.push(msg);
    }
    queue_condvar.notify_one();
}

void MQTTClient::ProcessMessage()
{
    while (true) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        queue_condvar.wait(lock, [this] {
            return !message_queue.empty() || mqtt_client_stop;
        });

        if (mqtt_client_stop && message_queue.empty()) {
            break;
        }

        MQTTMessage msg = message_queue.front();
        message_queue.pop();
        lock.unlock();  

        if (message_handler) {
            message_handler(msg);
        }
    }
}

void MQTTClient::connlost(void* context, char* cause)
{
    auto* self = static_cast<MQTTClient*>(context);
    printf("Connection lost. Cause: %s\n", cause ? cause : "unknown");

    self->mqtt_client_connected = false;

    if (self->disconnect_handler) {
        self->disconnect_handler(-1);
    }

    self->reconnect_delay = 1000; 
    self->reconnectWithBackoff();
}

int MQTTClient::msgarrvd(void* context, char* topicName, int /*topicLen*/, MQTTAsync_message* message)
{
    auto* self = static_cast<MQTTClient*>(context);

    std::string topic(topicName);
    std::string payload(static_cast<char*>(message->payload), static_cast<size_t>(message->payloadlen));

    self->enqueueMessage(topic, payload, message->qos, message->retained != 0);

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;   
}

void MQTTClient::onConnect(void* context, MQTTAsync_successData* /*response*/)
{
    auto* self = static_cast<MQTTClient*>(context);
    printf("Connected.\n");

    self->mqtt_client_connected     = true;
    self->reconnect_delay = 1000;   
    self->resubscribeAll();

    if (self->connect_handler) {
        self->connect_handler();
    }
}

void MQTTClient::onConnectFailure(void* context, MQTTAsync_failureData* response)
{
    auto* self = static_cast<MQTTClient*>(context);
    printf("Connect failed, rc=%d\n", response ? response->code : -1);

    self->mqtt_client_connected = false;
    self->reconnectWithBackoff();
}

void MQTTClient::onDisconnect(void* context, MQTTAsync_successData* /*response*/)
{
    auto* self = static_cast<MQTTClient*>(context);
    printf("Disconnected.\n");
    self->mqtt_client_connected = false;
}

void MQTTClient::onDisconnectFailure(void* context, MQTTAsync_failureData* response)
{
    printf("Disconnect failed, rc=%d\n", response ? response->code : -1);
}

void MQTTClient::onSubscribe(void* /*context*/, MQTTAsync_successData* response)
{
    printf("Subscribe succeeded, grantedQoS=%d\n",
           response ? response->alt.qos : -1);
}

void MQTTClient::onSubscribeFailure(void* /*context*/, MQTTAsync_failureData* response)
{
    printf("Subscribe failed, rc=%d\n", response ? response->code : -1);
}

void MQTTClient::onPublish(void* /*context*/, MQTTAsync_successData* /*response*/)
{
    printf("Publish succeeded.\n");
}

void MQTTClient::onPublishFailure(void* /*context*/, MQTTAsync_failureData* response)
{
    printf("Publish failed, rc=%d\n", response ? response->code : -1);
}