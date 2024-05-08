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
#include "LedSequencer.h"
#include "EventMonitor.h"
#include "ota.h"

#define PIN_SWITCH              D9
#define SW_LONG_PUSH_DURATION   10 // sec
#define DEBUG_REPORT_INTERVAL   3600  // sec

static pixel_state_t pixelState;

volatile bool swState;
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
  else if (msg.startsWith("STA:") && msg.length() == 5) {
    uint8_t state = msg.charAt(4) - '0';

    if (state & STATE_BRIGHT) {
      if (state & STATE_DAYTIME) {
        ledSeqSetLights(0);
      }
      else {
        ledSeqSetLights(255);
      }
    }
    else {
      ledSeqSetLights(8);
    }
  }
}

void onLocalEvent(uint16_t state)
{
  // Remote側へ状態通知
  String msg = "STA:";
  msg += String(state);
  mqttPublishEvent(msg.c_str());

  if (state & STATE_BRIGHT) {
    if (state & STATE_DAYTIME) {
      ledCtrlSetStreetLight(0);
    }
    else {
      ledCtrlSetStreetLight(255);
    }
  }
  else {
    ledCtrlSetStreetLight(8);
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
  ledSeqSetLights(255);

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
    swTicker.attach(SW_LONG_PUSH_DURATION, swLongPushHandler);
  }
  else {
    swTicker.detach();
  }
}

void swLongPushHandler(void)
{
  printLog("Erase SSID");
  mqttWifiClearSetting();

  // Let user know that SSID is erased
  ledSeqSetPixelHue(HUE_RED, 0xffff);
  ESP.restart();
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
