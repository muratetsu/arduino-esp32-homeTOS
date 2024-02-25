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

void ledTimerHandler(void)
{
  static uint8_t val = 0;

  if (pixelState.dulation > 0) {
    pixelState.dulation--;
    if (val < pixelState.val) {
      val++;
      pixels.setPixelColor(0, pixels.ColorHSV(pixelState.hue, pixelState.sat, val));
      pixels.show();
    }
  }
  else {
    if (val > 0) {
      val--;
      pixels.setPixelColor(0, pixels.ColorHSV(pixelState.hue, pixelState.sat, val));
      pixels.show();
    }
    else {
      ledTicker.detach();
    }
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
}

void ledCtrlSetPixel(pixel_state_t px)
{
  pixelState.dulation = px.dulation / LED_TIMER_INTERVAL;
  pixelState.hue      = px.hue;
  pixelState.sat      = px.sat;
  pixelState.val      = px.val;
  
  ledTicker.attach_ms(LED_TIMER_INTERVAL, ledTimerHandler);
}
