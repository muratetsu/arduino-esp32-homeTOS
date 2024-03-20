// LED Control
//
// February 4, 2024
// Tetsu Nishimura

#include <Arduino.h>
#include <Ticker.h>
#include <Adafruit_NeoPixel.h>
#include "LedControl.h"

// Digital LED
#define PIN_PIXEL           D10
#define NUM_PIXELS          1
#define LED_TIMER_INTERVAL  20

// LED
#define PIN_STREET_LIGHT    D2
#define PIN_ENTRANCE_LIGHT  D3
#define PIN_ROOM_LIGHT      D4

#define HUE_STEP            256

Ticker ledTicker;
Adafruit_NeoPixel pixels(NUM_PIXELS, PIN_PIXEL, NEO_GRB + NEO_KHZ800);
static pixel_state_t pxTarget;
led_val_t targetVal;

void ledTimerHandler(void)
{
  static pixel_state_t pxNow = {0};
  static led_val_t val = {0};
  int32_t hueDiff;
  bool changed = false;

  // Digital LED Control
  if (pxTarget.duration > 0) {
    pxTarget.duration--;
  }
  else {  // If the duration expired, set target val to zero
    pxTarget.val = 0;
  }

  hueDiff = ((pxTarget.hue - pxNow.hue + 0x8000L) & 0xffff) - 0x8000L;
  if (hueDiff > HUE_STEP) {
    pxNow.hue += HUE_STEP;
    changed = true;
  }
  else if (hueDiff < - HUE_STEP) {
    pxNow.hue -= HUE_STEP;
    changed = true;
  }

  if (pxNow.sat < pxTarget.sat) {
    pxNow.sat++;
    changed = true;
  }
  else if (pxNow.sat > pxTarget.val) {
    pxNow.sat--;
    changed = true;
  }

  if (pxNow.val < pxTarget.val) {
    pxNow.val++;
    changed = true;
  }
  else if (pxNow.val > pxTarget.val) {
    pxNow.val--;
    changed = true;
  }

  if (changed) {
    pixels.setPixelColor(0, pixels.ColorHSV(pxNow.hue, pxNow.sat, pxNow.val));
    pixels.show();
  }

  // Analog LEDs Control
  if (val.street < targetVal.street) {
    analogWrite(PIN_STREET_LIGHT, ++val.street);
  }
  else if (val.street > targetVal.street) {
    analogWrite(PIN_STREET_LIGHT, --val.street);
  }

  if (val.entrance < targetVal.entrance) {
    analogWrite(PIN_ENTRANCE_LIGHT, ++val.entrance);
  }
  else if (val.entrance > targetVal.entrance) {
    analogWrite(PIN_ENTRANCE_LIGHT, --val.entrance);
  }

  if (val.room < targetVal.room) {
    analogWrite(PIN_ROOM_LIGHT, ++val.room);
  }
  else if (val.room > targetVal.room) {
    analogWrite(PIN_ROOM_LIGHT, --val.room);
  }
}

void ledCtrlInit(void)
{
  pixels.begin();
  pinMode(PIN_STREET_LIGHT, OUTPUT);
  pinMode(PIN_ENTRANCE_LIGHT, OUTPUT);
  pinMode(PIN_ROOM_LIGHT, OUTPUT);

  analogWrite(PIN_STREET_LIGHT, 128);
  analogWrite(PIN_ENTRANCE_LIGHT, 128);
  analogWrite(PIN_ROOM_LIGHT, 128);

  ledTicker.attach_ms(LED_TIMER_INTERVAL, ledTimerHandler);
}

void ledCtrlSetPixel(pixel_state_t px)
{
  pxTarget.duration = px.duration / LED_TIMER_INTERVAL;
  pxTarget.hue      = px.hue;
  pxTarget.sat      = px.sat;
  pxTarget.val      = px.val;
}

void ledCtrlSetPixelHue(uint16_t hue, uint16_t duration)
{
  pxTarget.duration = duration / LED_TIMER_INTERVAL;
  pxTarget.hue = hue;
  pxTarget.sat = 255;
  pxTarget.val = 255;
}

void ledCtrlSetStreetLight(uint8_t val)
{
  targetVal.street = val;
}

void ledCtrlSetEntranceLight(uint8_t val)
{
  targetVal.entrance = val;
}

void ledCtrlSetRoomLight(uint8_t val)
{
  targetVal.room = val;
}
