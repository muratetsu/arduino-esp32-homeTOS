// Project "Home the Other Side"
// 向こうのお家プロジェクト
//
// February 3, 2024
// Tetsu Nishimura

#include <stdlib.h>
#include <Ticker.h>
#include <time.h>
#include "Mqtt.h"
#include "LedSequencer.h"
#include "LedControl.h"
#include "EventMonitor.h"
#include "ota.h"

#define PIN_SWITCH              D9
#define SW_LONG_PUSH_DURATION   10 // sec
#define DEBUG_REPORT_INTERVAL   3600  // sec

static pixel_state_t pixelState;

volatile bool swState;
volatile bool swLongPushed;
Ticker swTicker;
Ticker debugReportTicker;

//*******************************************************
// Event Handling

void onRemoteEvent(const String& msg)
{
  printLog("Remote event received");

  if (msg.startsWith("BTN:") && msg.length() == 16) {
    char buf[20] = {0};
    long long val;
    pixel_state_t px;
    uint16_t duration;

    msg.getBytes((unsigned char*)buf, sizeof(buf));
    val = strtoll(&buf[4], NULL, 16);
    duration = (val >> 32) & 0xffff;
    px.hue   = (val >> 16) & 0xffff;
    px.sat   = (val >>  8) & 0xff;
    px.val   =  val        & 0xff;
    ledSeqSetPixel(px, duration);
  }
  else if (msg == "LUM:0") {
    ledCtrlSetEntranceLight(0);
    ledCtrlSetRoomLight(0);
  }
  else if (msg == "LUM:1") {
    ledCtrlSetEntranceLight(255);
    ledCtrlSetRoomLight(255);
  }
}

void onLocalEvent(uint16_t state)
{
  String msg;

  if (state & STATE_DAYTIME) {
    msg = "Daytime - ";
    ledCtrlSetStreetLight(0);
    // Remote側へ状態通知
    mqttPublishEvent("LUM:0");
  }
  else {
    msg = "Nighttime - ";

    if (state & STATE_BRIGHT) {
      ledCtrlSetStreetLight(255);
      // Remote側へ状態通知
      mqttPublishEvent("LUM:1");
    }
    else {
      ledCtrlSetStreetLight(16);
      // Remote側へ状態通知
      mqttPublishEvent("LUM:0");
    }
  }

  if (state & STATE_BRIGHT) {
    msg += "Bright";
  }
  else {
    msg += "Dark";
  }

  printLog(msg.c_str());
}

void debugReportHandler(void)
{
  eventState_t state;
  char str[256];

  eventMonitorGetStatus(&state);
  sprintf(str, "Brightness: %d, %d", state.min, state.max);
  mqttPublishState(str);
}

//*******************************************************
// Arduino setup and loop

void setup()
{
  pinMode(PIN_SWITCH, INPUT);
  Serial.begin(115200);

  otaInit();

  ledCtrlInit();
  ledCtrlSetStreetLight(255);
  ledCtrlSetEntranceLight(255);
  ledCtrlSetRoomLight(255);

  attachInterrupt(PIN_SWITCH, swHandler, CHANGE);

  mqttWifiSetup();
  mqttInit(onRemoteEvent);

  eventMonitorInit(onLocalEvent);
  debugReportTicker.attach(DEBUG_REPORT_INTERVAL, debugReportHandler);
}

void loop()
{
  char buf[20];

  mqttLoop();

  if (swState) {
    swState = false;
    uint16_t duration = 4000;
    pixelState.hue = random(65536);
    pixelState.sat = 128 + random(128);
    pixelState.val = 128;
    ledSeqSetPixel(pixelState, duration);

    pixelEncode(buf, pixelState, duration);
    mqttPublishEvent(buf);
    printLog(buf);
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
  printLog("Erase SSID");

  mqttWifiClearSetting();

  // Let user know that SSID is erased
  // TODO: 高速点滅など分かりやすいです表示に変えること
  pixelState.hue = 0;
  pixelState.sat = 255;
  pixelState.val = 128;
  ledSeqSetPixel(pixelState, 2000);
}

void pixelEncode(char* buf, pixel_state_t px, uint16_t duration)
{
    sprintf(buf, "BTN:%04X%04X%02X%02X",
      duration, px.hue, px.sat, px.val);
}

void printLog(const char* msg)
{
  time_t t;
  struct tm *tm;
  char str[256];

  t = time(NULL);
  tm = localtime(&t);
  sprintf(str, "%04d/%02d/%02d %02d:%02d:%02d %s", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, msg);
  Serial.println(str);
}
