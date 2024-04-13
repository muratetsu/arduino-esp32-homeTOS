// MQTT
//
// March 3, 2024
// Tetsu Nishimura
#ifndef MQTT_H
#define MQTT_H

#include "EspMQTTClient.h"

bool mqttWifiSetup(void);
void mqttWifiClearSetting(void);
void mqttInit(MessageReceivedCallback callback);
void mqttPublishEvent(const String &payload);
void mqttLoop(void);
bool isWifiConnected(void);
void mqttPublishState(const char* msg);

#endif  // MQTT_H
