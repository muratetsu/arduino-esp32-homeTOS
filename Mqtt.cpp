// MQTT
//
// March 3, 2024
// Tetsu Nishimura

#include <Arduino.h>
#include <Preferences.h>
#include <time.h>
#include "Mqtt.h"
#include "LedSequencer.h"
#include "ota.h"

#define JST       (3600 * 9)

// Function prototype
bool wifiIsSsidStored(void);
void generateTopic(char* topic, const char* id, const char* func);
void onHostConfig(const String& msg);
void publishState(void);

typedef struct {
  uint8_t debug;        // Debug on yes/no 1/0
  char nodename[32];    // this node name
  char ssid[32];        // WiFi SSID
  char password[64];    // WiFi Password
  char mqttbroker[32];  // MQTT broker URL
  uint16_t mqttport;    // MQTT port
  char mqttuser[32];    // MQTT Username
  char mqttpass[64];    // MQTT Password
  char serviceName[32]; // Service Name
} configuration_t;

configuration_t CONFIGURATION = {
  1,
  "NodeName",
  "WiFiSSID",
  "WifiPassword",
  "broker.emqx.io",
  1883,
  "emqx",
  "public",
  "htos-f61ca251"
};

// MQTT Broker
char topicState[64];      // topic to publish local state
char topicConfig[64];     // topic to subscribe configuration data
char topicLocalEvent[64];  // topic to publish local event
char topicRemoteEvent[64]; // topic to subscribe remote event
String local_id;
String remote_id;

EspMQTTClient client;          // using the default constructor
Preferences prefs;

MessageReceivedCallback remoteEventCallback;


bool mqttWifiSetup(void)
{
  unsigned long tm;
  
  if (!wifiIsSsidStored()) {
    Serial.println("No SSID stored");
    ledSeqBlinkPixel(HUE_RED, 2000, 0xffff);
    //Init WiFi as Station, start SmartConfig
    WiFi.mode(WIFI_AP_STA);
    WiFi.beginSmartConfig();
  
    //Wait for SmartConfig packet from mobile
    Serial.print("Waiting for SmartConfig");
    tm = millis();
    while (!WiFi.smartConfigDone()) {
      delay(500);
      Serial.print(".");

      // 5分以上SmartConfigによる設定がされなかったら抜ける
      if (millis() - tm > 300000) {
        Serial.println("");
        Serial.println("Timeout");
        ESP.restart();
      }
    }
    Serial.println("");
    Serial.println("SmartConfig received.");

    // Store SSID and password
    // SmartConfigで設定されたSSIDとPasswordを別のネームスペースに移しておく
    char wifi_ssid[37] = {};
    char wifi_key[66] = {};

    prefs.begin("nvs.net80211", true);
    prefs.getBytes("sta.ssid", wifi_ssid, sizeof(wifi_ssid));
    prefs.getBytes("sta.pswd", wifi_key, sizeof(wifi_key));
    prefs.end();

    strcpy(CONFIGURATION.ssid, &wifi_ssid[4]);
    strcpy(CONFIGURATION.password, wifi_key);

    prefs.begin("wi-fi", false);
    prefs.putBytes("sta.ssid", CONFIGURATION.ssid, sizeof(CONFIGURATION.ssid));
    prefs.putBytes("sta.pswd", CONFIGURATION.password, sizeof(CONFIGURATION.password));
    prefs.end();
  }

  return true;
}

void mqttWifiClearSetting(void)
{
  prefs.begin("wi-fi", false);
  prefs.clear();
  prefs.end();
}

void mqttInit(MessageReceivedCallback callback)
{
  remoteEventCallback = callback;

  ledSeqSetPixelHue(HUE_RED, 0xffff);

  prefs.begin("wi-fi", true);
  prefs.getBytes("sta.ssid", CONFIGURATION.ssid, sizeof(CONFIGURATION.ssid));
  prefs.getBytes("sta.pswd", CONFIGURATION.password, sizeof(CONFIGURATION.password));
  prefs.end();

  local_id = WiFi.macAddress();
  local_id.replace(":", "");
  String loginId = CONFIGURATION.serviceName;
  loginId += "-" + local_id;
  strcpy(CONFIGURATION.nodename, loginId.c_str());

  // Load remote device address
  prefs.begin("app");
  remote_id = prefs.getString("remoteAddr", "000000000000");
  prefs.end();

  // Create topics
  generateTopic(topicState, local_id.c_str(), "state");
  generateTopic(topicConfig, local_id.c_str(), "config");
  generateTopic(topicLocalEvent, local_id.c_str(), "event");
  generateTopic(topicRemoteEvent, remote_id.c_str(), "event");


  if (CONFIGURATION.debug) {
    client.enableDebuggingMessages();
  }
  client.setWifiCredentials(CONFIGURATION.ssid, CONFIGURATION.password);
  client.setMqttClientName(CONFIGURATION.nodename);
  client.setMqttServer(CONFIGURATION.mqttbroker, CONFIGURATION.mqttuser, CONFIGURATION.mqttpass, CONFIGURATION.mqttport);
  client.setKeepAlive(60); // Change the keep alive interval (15 seconds by default)
}

void mqttPublishEvent(const String &payload)
{
    client.publish(topicLocalEvent, payload, true); // Retain as published
}

void mqttLoop(void)
{
  client.loop();
}

void onConnectionEstablished()
{
  // Subscribe topics
  client.subscribe(topicRemoteEvent, remoteEventCallback);
  client.subscribe(topicConfig, onHostConfig);

  // Publish state message
  publishState();

  // Get time from NTP server
  configTime( JST, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");
  
  // Turn the Digital LED green then off
  ledSeqSetPixelHue(HUE_GREEN, 0);
}

void onHostConfig(const String& msg)
{
  Serial.println("Configuration data received");

  if (msg.length() == 12) {
    if (remote_id != msg) {
      remote_id = msg;

      // Store remote device address
      prefs.begin("app");
      prefs.putString("remoteAddr", remote_id);
      prefs.end();

      client.unsubscribe(topicRemoteEvent);      
      generateTopic(topicRemoteEvent, remote_id.c_str(), "event");
      client.subscribe(topicRemoteEvent, remoteEventCallback);
      
      publishState();
    }
  }
}

bool wifiIsSsidStored(void)
{
  bool ret = true;
  size_t ssidLen, pswdLen;

  prefs.begin("wi-fi", true);
  ssidLen = prefs.getBytesLength("sta.ssid");
  pswdLen = prefs.getBytesLength("sta.ssid");
  prefs.end();

  if (ssidLen == 0 || pswdLen == 0) {
    ret = false;
  }

  return ret;
}

void generateTopic(char* topic, const char* id, const char* func)
{
  String s;
  
  s = CONFIGURATION.serviceName;
  s += "/";
  s += id;
  s += "/";
  s += func;
  
  strcpy(topic, s.c_str());
  Serial.println(topic);
}

void publishState(void)
{
  String msg;
  msg  = "Rev:";
  msg += firmwareHash;
  msg += ", ";
  msg += "Peer:" + remote_id;
  
  client.publish(topicState, msg.c_str());
}

bool isWifiConnected(void)
{
  return client.isWifiConnected();
}

void mqttPublishState(const char* msg)
{
  client.publish(topicState, msg);
}