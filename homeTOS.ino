// Project "Home the Other Side"
// 向こうのお家プロジェクト
//
// February 3, 2024
// Tetsu Nishimura

#include <stdlib.h>
#include <Ticker.h>
#include <Preferences.h>
#include "EspMQTTClient.h"
#include "LedControl.h"
#include "Sensor.h"

#define PIN_SWITCH              D9
#define SW_LONG_PUSH_DURATION   10 // sec

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

static pixel_state_t pixelState;

EspMQTTClient client;          // using the default constructor
Preferences prefs;
volatile bool swState;
volatile bool swLongPushed;
Ticker swTicker;

//*******************************************************
// Network Setting

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

bool wifiSetup(void)
{
  unsigned long tm;
  
  if (!wifiIsSsidStored()) {
    Serial.println("No SSID stored");
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

void mqttInit(void)
{
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


  if (CONFIGURATION.debug)
    client.enableDebuggingMessages();

  client.setWifiCredentials(CONFIGURATION.ssid, CONFIGURATION.password);
  client.setMqttClientName(CONFIGURATION.nodename);
  client.setMqttServer(CONFIGURATION.mqttbroker, CONFIGURATION.mqttuser, CONFIGURATION.mqttpass, CONFIGURATION.mqttport);
}

void onConnectionEstablished()
{
  // Subscribe topics
  client.subscribe(topicRemoteEvent, onRemoteEvent);
  client.subscribe(topicConfig, onHostConfig);

  // Publish state message
  mqttPublishState();

  // Turn off Digital LED
  ledCtrlSetPixelOff();
}

void onRemoteEvent(const String& msg)
{
  Serial.println("Remote event received");

  if (msg.startsWith("BTN:")) {
    char buf[20] = {0};
    long long val;
    pixel_state_t px;

    msg.getBytes((unsigned char*)buf, sizeof(buf));
    val = strtoll(&buf[4], NULL, 16);
    px.dulation = (val >> 32) & 0xffff;
    px.hue      = (val >> 16) & 0xffff;
    px.sat      = (val >>  8) & 0xff;
    px.val      =  val        & 0xff;
    ledCtrlSetPixel(px);
  }
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
      client.subscribe(topicRemoteEvent, onRemoteEvent);
      
      mqttPublishState();
    }
  }
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

void mqttPublishState(void)
{
  String msg = "Peer: " + remote_id;
  
  client.publish(topicState, msg.c_str());
}

//*******************************************************
// Arduino setup and loop

void setup()
{
  pinMode(PIN_SWITCH, INPUT);
  Serial.begin(115200);

  ledCtrlInit();
  ledCtrlSetPixelRed();
  ledCtrlSetStreetLight(255);
  ledCtrlSetEntranceLight(255);
  ledCtrlSetRoomLight(255);

  attachInterrupt(PIN_SWITCH, swHandler, CHANGE);

  wifiSetup();
  mqttInit();

  // sensorInit();
}

void loop()
{
  char buf[20];

  client.loop();

  if (swState) {
    swState = false;
    pixelState.dulation = 2000;
    pixelState.hue = random(65536);
    pixelState.sat = 128 + random(128);
    pixelState.val = 128;
    ledCtrlSetPixel(pixelState);

    pixelEncode(buf, pixelState);
    client.publish(topicLocalEvent, buf);
    Serial.println(buf);
  }
}

void ARDUINO_ISR_ATTR swHandler(void)
{
  if (digitalRead(PIN_SWITCH) == LOW) {
    swState = true;
    swLongPushed = false;
    swTicker.attach(SW_LONG_PUSH_DURATION, swLongPushHandler);
  }
  else {
    swTicker.detach();
    if (swLongPushed) {
      ESP.restart();
    }
  }
}

void swLongPushHandler(void)
{
  swLongPushed = true;
  Serial.println("Erase SSID");

  prefs.begin("wi-fi", false);
  prefs.clear();
  prefs.end();

  // Let user know that SSID is erased
  // TODO: 高速点滅など分かりやすいです表示に変えること
  pixelState.dulation = 2000;
  pixelState.hue = 0;
  pixelState.sat = 255;
  pixelState.val = 128;
  ledCtrlSetPixel(pixelState);
}

void pixelEncode(char* buf, pixel_state_t px)
{
    sprintf(buf, "BTN:%04X%04X%02X%02X",
      px.dulation, px.hue, px.sat, px.val);
}
