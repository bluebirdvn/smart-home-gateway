#ifndef _MQTT_MESSAGE_H
#define _MQTT_MESSAGE_H

#include "mqtt_connection_config.h"
#include <cstdint>

struct MQTTMessage {
	std::string topic;
	MQTTQoS qos;
	std::string payload;
	bool retain;
	uint32_t timeStamp;
};

#endif