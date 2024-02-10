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
    if (pixelState.val > 0) {
      pixelState.val--;
    }
    pixels.setPixelColor(0, pixels.ColorHSV(pixelState.hue, pixelState.sat, pixelState.val));
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
  pixelState.hue      = px.hue;
  pixelState.sat      = px.sat;
  pixelState.val      = px.val;

  pixels.setPixelColor(0, pixels.ColorHSV(px.hue, px.sat, px.val));
  pixels.show();
}
