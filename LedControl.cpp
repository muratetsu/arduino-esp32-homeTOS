// LED Control
//
// February 4, 2024
// Tetsu Nishimura

#include <Arduino.h>
#include <Ticker.h>
#include <Adafruit_NeoPixel.h>
#include "LedControl.h"

#define PIXEL_PIN   10
#define NUM_PIXELS   1

Ticker ledTicker;
Adafruit_NeoPixel pixels(NUM_PIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
static pixel_state_t pixelState;

void ledTimerHandler(void)
{
  if (pixelState.dulation > 0) {
    pixelState.dulation--;
  }
  else {
    if (pixelState.red > 0) {
      pixelState.red--;
    }
    if (pixelState.green > 0) {
      pixelState.green--;
    }
    if (pixelState.blue > 0) {
      pixelState.blue--;
    }
    pixels.setPixelColor(0, pixelState.red, pixelState.green, pixelState.blue);
    pixels.show();
  }
}

void ledCtrlInit(void)
{
  pixels.begin();
  ledTicker.attach_ms(10, ledTimerHandler);
}

void ledCtrlSetPixel(pixel_state_t px)
{
  pixelState.dulation = px.dulation;
  pixelState.red      = px.red;
  pixelState.green    = px.green;
  pixelState.blue     = px.blue;

  pixels.setPixelColor(0, px.red, px.green, px.blue);
  pixels.show();
}
