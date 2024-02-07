// Project "Home the Other Side"
// 向こうのお家プロジェクト
//
// February 3, 2024
// Tetsu Nishimura

#include <Preferences.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "LedControl.h"

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

WiFiClient espClient;
PubSubClient client(espClient);
Preferences prefs;

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
    // ここでLEDの制御を行う
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
  ledCtrlSetPixel(1000, 128, 0, 0);

//  wifiDumpSsidAndPassword();
  wifiConnect();
  mqttConnect();

  ledCtrlSetPixel(0, 128, 0, 0);
}

void loop()
{
  static int count = 0;
  static int swLastState = 1;
  char buf[20];

  client.loop();

  int swState = digitalRead(PushSw);
  if (swLastState == 1 && swState == 0) {
    sprintf(buf, "Pushed %d", count++);
    client.publish(topicLocalEvent, buf);
    Serial.println(buf);
    ledCtrlSetPixel(100, 0, 128, 128);
  }
  swLastState = swState;
}
