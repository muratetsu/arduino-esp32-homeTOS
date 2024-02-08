// LED Control
//
// February 4, 2024
// Tetsu Nishimura

#include <Ticker.h>
#include <Adafruit_NeoPixel.h>
#include "LedControl.h"

#define PIN         10
#define NUMPIXELS   1

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
Ticker ticker;
static pixel_state_t pixelState;

void timerHandler(void)
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
  ticker.attach_ms(10, timerHandler);
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
