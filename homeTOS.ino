// Project "Home the Other Side"
// 向こうのお家プロジェクト
//
// February 3, 2024
// Tetsu Nishimura

#include <stdlib.h>
#include <Ticker.h>
#include <time.h>
#include "Mqtt.h"
#include "LedControl.h"
#include "EventMonitor.h"
#include "HttpUpdate.h"

#define PIN_SWITCH              D9
#define SW_LONG_PUSH_DURATION   10 // sec

static pixel_state_t pixelState;

volatile bool swState;
volatile bool swLongPushed;
Ticker swTicker;

//*******************************************************
// Event Handling

void onRemoteEvent(const String& msg)
{
  printLog("Remote event received");

  if (msg.startsWith("BTN:")) {
    char buf[20] = {0};
    long long val;
    pixel_state_t px;

    msg.getBytes((unsigned char*)buf, sizeof(buf));
    val = strtoll(&buf[4], NULL, 16);
    px.duration = (val >> 32) & 0xffff;
    px.hue      = (val >> 16) & 0xffff;
    px.sat      = (val >>  8) & 0xff;
    px.val      =  val        & 0xff;
    ledCtrlSetPixel(px);
  }
  else if (msg.startsWith("LED:")) {
    char buf[20] = {0};
    long val;

    msg.getBytes((unsigned char*)buf, sizeof(buf));
    val = strtol(&buf[4], NULL, 16);
    ledCtrlSetEntranceLight((val >> 8) & 0xff);
    ledCtrlSetRoomLight(val & 0xff);
  }
}

void onLocalEvent(uint16_t state)
{
  String msg;

  if (state & STATE_DAYTIME) {
    msg = "Daytime - ";
    ledCtrlSetStreetLight(0);
    // Remote側へ状態通知
    mqttPublishEvent("LED:0000");
  }
  else {
    msg = "Nighttime - ";

    if (state & STATE_BRIGHT) {
      ledCtrlSetStreetLight(255);
      // Remote側へ状態通知
      mqttPublishEvent("LED:FFFF");
    }
    else {
      ledCtrlSetStreetLight(16);
      // Remote側へ状態通知
      mqttPublishEvent("LED:1000");
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

//*******************************************************
// Arduino setup and loop

void setup()
{
  pinMode(PIN_SWITCH, INPUT);
  Serial.begin(115200);

  httpUpdateInit();

  ledCtrlInit();
  ledCtrlSetStreetLight(255);
  ledCtrlSetEntranceLight(255);
  ledCtrlSetRoomLight(255);

  attachInterrupt(PIN_SWITCH, swHandler, CHANGE);

  mqttWifiSetup();
  mqttInit(onRemoteEvent);

  eventMonitorInit(onLocalEvent);
}

void loop()
{
  char buf[20];

  mqttLoop();

  if (swState) {
    swState = false;
    pixelState.duration = 2000;
    pixelState.hue = random(65536);
    pixelState.sat = 128 + random(128);
    pixelState.val = 128;
    ledCtrlSetPixel(pixelState);

    pixelEncode(buf, pixelState);
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
  pixelState.duration = 2000;
  pixelState.hue = 0;
  pixelState.sat = 255;
  pixelState.val = 128;
  ledCtrlSetPixel(pixelState);
}

void pixelEncode(char* buf, pixel_state_t px)
{
    sprintf(buf, "BTN:%04X%04X%02X%02X",
      px.duration, px.hue, px.sat, px.val);
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
