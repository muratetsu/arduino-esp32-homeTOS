// LED Control
//
// February 4, 2024
// Tetsu Nishimura

#include <Ticker.h>
#include <Adafruit_NeoPixel.h>
#include "LedControl.h"

#define PIN         10
#define NUMPIXELS   1

typedef struct {
  int dulation;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} pixel_state_t;

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
Ticker ticker;
pixel_state_t pixelState;

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

void ledCtrlSetPixel(int dulation, uint8_t red, uint8_t green, uint8_t blue)
{
  pixelState.dulation = dulation;
  pixelState.red      = red;
  pixelState.green    = green;
  pixelState.blue     = blue;  

  pixels.setPixelColor(0, red, green, blue);
  pixels.show();
}
