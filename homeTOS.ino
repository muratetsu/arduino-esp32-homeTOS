// Project "Home the Other Side"
// 向こうのお家プロジェクト
//
// February 3, 2024
// Tetsu Nishimura

#include <Preferences.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <stdlib.h>
#include "LedControl.h"
#include "Sensor.h"

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int mqtt_port = 1883;
const char *serviceName = "htos-f61ca251";
char topicState[64];      // topic to publish local state
char topicConfig[64];     // topic to subscribe configuration data
char topicLocalEvent[64];  // topic to publish local event
char topicRemoteEvent[64]; // topic to subscribe remote event
String local_id;
String remote_id;

const int PushSw = 9; // Boot Switch
static pixel_state_t pixelState;

WiFiClient espClient;
PubSubClient client(espClient);
Preferences prefs;
volatile bool buttonState;

//*******************************************************
// WiFi

// Dump WiFi SSID and PASSWORD (for debug purpose)
void wifiDumpSsidAndPassword(void)
{
  char wifi_ssid[37] = {};
  char wifi_key[66] = {};

  prefs.begin("nvs.net80211", true);
  prefs.getBytes("sta.ssid", wifi_ssid, sizeof(wifi_ssid));
  prefs.getBytes("sta.pswd", wifi_key, sizeof(wifi_key));
  prefs.end();
  
  Serial.printf("sta.ssid: %s\n", &wifi_ssid[4]);
  Serial.printf("sta.pswd: %s\n", wifi_key);
}

// Establish WiFi connection
//
// TODO:
// 今はWiFi APに一定時間接続できなかったらSmartConfigを実行するようにしているが，
// APの調子が悪かったから繋がらなかっただけで新しいAPの設定をしたいわけではない，みたいな状況はありそう．
// ボタンの長押しなど，ユーザの明確な意思表示があったときだけSmartConfigを実行するようにすべきかも．
// Bootボタン長押しで，SSID削除→SmartConfig起動，としてみる？
bool wifiConnect(void)
{
  unsigned long tm;
  
  // 前回接続時の情報で接続する
  Serial.print("Connecting to WiFi AP");
  WiFi.begin();
  tm = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - tm < 10000) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");

  // If WiFi connection failed, start SmartConfig
  if (WiFi.status() != WL_CONNECTED) {
    //Init WiFi as Station, start SmartConfig
    WiFi.mode(WIFI_AP_STA);
    WiFi.beginSmartConfig();
  
    //Wait for SmartConfig packet from mobile
    Serial.print("Waiting for SmartConfig");
    tm = millis();
    while (!WiFi.smartConfigDone()) {
      delay(500);
      Serial.print(".");

      // 5分以上SmartConfigによる設定がされなかったら抜ける(発熱の懸念があるので)
      if (millis() - tm > 300000) {
        Serial.println("");
        Serial.println("Timeout");
        return false;
      }
    }
    Serial.println("");
    Serial.println("SmartConfig received.");
  
    //Wait for WiFi to connect to AP
    Serial.print("Connectiong to WiFi AP");
    tm = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      // 繋がるはずだけど，10秒以上繋がらなかったら念の為リセットする
      if (millis() - tm > 10000) {
        Serial.println("");
        Serial.println("Timeout. Restart ESP32.");
        ESP.restart();
      }
    }  
  }
  
  Serial.println("Connection established.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  return true;
}

//*******************************************************
// MQTT

void mqttConnect(void)
{
  //connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(mqttCallback);

  local_id = WiFi.macAddress();
  local_id.replace(":", "");

  String loginId = serviceName;
  loginId += "-" + local_id;

  while (!client.connected()) {
    Serial.printf("%s connects to MQTT broker\n", loginId.c_str());
    if (client.connect(loginId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  // Load remote device address
  prefs.begin("app");
  remote_id = prefs.getString("remoteAddr", "000000000000");
  prefs.end();

  // Create topics
  generateTopic(topicState, local_id.c_str(), "state");
  generateTopic(topicConfig, local_id.c_str(), "config");
  generateTopic(topicLocalEvent, local_id.c_str(), "event");
  generateTopic(topicRemoteEvent, remote_id.c_str(), "event");

  // Subscribe topics
  client.subscribe(topicRemoteEvent);
  client.subscribe(topicConfig);

  // Publish state message
  mqttPublishState();
}

void generateTopic(char* topic, const char* id, const char* func)
{
  String s;
  
  s = serviceName;
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
  Serial.println(msg);
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  if (strcmp(topic, topicConfig) == 0) {
    Serial.println("Configuration data received");
    mqttReplaceRemoteId(payload, length);
  }
  
  else if (strcmp(topic, topicRemoteEvent) == 0) {
    Serial.println("Remote event received");
    if (strncmp((char*)payload, "BTN:", 4) == 0) {
      pixel_state_t px;
      pixelDecode(payload, length, &px);
      ledCtrlSetPixel(px);
    }
  }
}

void mqttReplaceRemoteId(byte *payload, int length)
{
  char msg[13];
  
  if (length == 12) {
    memcpy(msg, payload, length);
    msg[12] = '\0';
    
    if (remote_id != msg) {
      remote_id = msg;

      // Store remote device address
      prefs.begin("app");
      prefs.putString("remoteAddr", remote_id);
      prefs.end();

      client.unsubscribe(topicRemoteEvent);      
      generateTopic(topicRemoteEvent, remote_id.c_str(), "event");
      client.subscribe(topicRemoteEvent);
      
      mqttPublishState();
    }
  }
}

//*******************************************************
// Arduino setup and loop

void setup()
{
  pinMode(PushSw, INPUT);
  Serial.begin(115200);

  ledCtrlInit();

  pixelState.dulation = 0xffff;
  pixelState.hue = 0;
  pixelState.sat = 255;
  pixelState.val = 128;
  ledCtrlSetPixel(pixelState);

//  wifiDumpSsidAndPassword();
  wifiConnect();
  mqttConnect();

  pixelState.dulation = 0;
  ledCtrlSetPixel(pixelState);

  // sensorInit();
  attachInterrupt(PushSw, buttonHandler, RISING);
}

void loop()
{
  char buf[20];

  client.loop();

  if (buttonState) {
    buttonState = false;
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

void ARDUINO_ISR_ATTR buttonHandler(void)
{
  buttonState = true;
}

void pixelEncode(char* buf, pixel_state_t px)
{
    sprintf(buf, "BTN:%04X%04X%02X%02X",
      px.dulation, px.hue, px.sat, px.val);
}

void pixelDecode(const byte* payload, int len, pixel_state_t* px)
{
  char buf[20] = {0};
  long long val;

  memcpy(buf, payload, len);
  Serial.println(buf);
  val = strtoll(&buf[4], NULL, 16);

  px->dulation = (val >> 32) & 0xffff;
  px->hue      = (val >> 16) & 0xffff;
  px->sat      = (val >>  8) & 0xff;
  px->val      =  val        & 0xff;
}