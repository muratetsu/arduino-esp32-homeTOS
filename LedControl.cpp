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

Ticker ledTicker;
Adafruit_NeoPixel pixels(NUM_PIXELS, PIN_PIXEL, NEO_GRB + NEO_KHZ800);
static pixel_state_t pixelState;
led_val_t targetVal;

void ledTimerHandler(void)
{
  static uint8_t valPixel = 0;
  static led_val_t val = {0};

  // Digital LED Control
  if (pixelState.duration > 0) {
    pixelState.duration--;
    if (valPixel < pixelState.val) {
      valPixel++;
      pixels.setPixelColor(0, pixels.ColorHSV(pixelState.hue, pixelState.sat, valPixel));
      pixels.show();
    }
  }
  else {
    if (valPixel > 0) {
      valPixel--;
      pixels.setPixelColor(0, pixels.ColorHSV(pixelState.hue, pixelState.sat, valPixel));
      pixels.show();
    }
    else {
      // Do nothing
    }
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
  pixelState.duration = px.duration / LED_TIMER_INTERVAL;
  pixelState.hue      = px.hue;
  pixelState.sat      = px.sat;
  pixelState.val      = px.val;  
}

void ledCtrlSetPixelHue(uint16_t hue, uint16_t duration)
{
  pixelState.duration = duration / LED_TIMER_INTERVAL;
  pixelState.hue = hue;
  pixelState.sat = 255;
  pixelState.val = 255;
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
