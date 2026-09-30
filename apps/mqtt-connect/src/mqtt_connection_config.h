#ifndef MQTT_CONNECTION_CONFIG_H
#define MQTT_CONNECTION_CONFIG_H

#include <string>
#include <cstdint>

enum MQTTQoS {
	QOS_0_AT_MOST_ONCE = 0,
	QOS_1_AT_LEAST_ONCE,
	QOS_2_EXACTLY_ONCE
};

struct MQTTLwtConfig {
	std::string willTopic;
	std::string willPayload;
	MQTTQoS willQoS;
	bool willRetain;
};

struct MQTTTlsConfig {
	std::string caFile;
	std::string certFile;
	std::string keyFile;
	std::string tlsVersion;
	bool verifyServer;
};

struct MQTTConnectionConfig {
	std::string brokerURL;
	uint16_t port;
	std::string clientID;
	std::string userName;
	std::string password;
	
	bool cleanSession;
	uint16_t keepAliveInterval;
	bool havingTLS;
	MQTTLwtConfig lwt;
	MQTTTlsConfig tls;
};


#endif