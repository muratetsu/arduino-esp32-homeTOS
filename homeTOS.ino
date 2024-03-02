// Project "Home the Other Side"
// 向こうのお家プロジェクト
//
// February 3, 2024
// Tetsu Nishimura

#include <stdlib.h>
#include <Ticker.h>
#include "Mqtt.h"
#include "LedControl.h"
#include "Sensor.h"

#define PIN_SWITCH              D9
#define SW_LONG_PUSH_DURATION   10 // sec

static pixel_state_t pixelState;

volatile bool swState;
volatile bool swLongPushed;
Ticker swTicker;

//*******************************************************
// Network

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

  mqttWifiSetup();
  mqttInit(onRemoteEvent);

  // sensorInit();
}

void loop()
{
  char buf[20];

  mqttLoop();

  if (swState) {
    swState = false;
    pixelState.dulation = 2000;
    pixelState.hue = random(65536);
    pixelState.sat = 128 + random(128);
    pixelState.val = 128;
    ledCtrlSetPixel(pixelState);

    pixelEncode(buf, pixelState);
    mqttPublishEvent(buf);
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

  mqttWifiClearSetting();

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
