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

void timerHandler(void)
{
  static int flag = 1;
  static int cnt = 0;

  if (flag) {
    if (++cnt == 128) {
      flag = 0;
    }
  }
  else {
    if (--cnt < 0) {
      flag = 1;
      cnt = 0;
    }
  }
  pixels.setPixelColor(0, pixels.Color(0, 0, cnt));
  pixels.show();
}

void ledCtrlInit(void)
{
  pixels.begin();
}

void ledCtrlStartTimer(void)
{
  ticker.attach_ms(10, timerHandler);
}

void ledCtrlSetPixel(pixel_state_t state)
{
  switch (state) {
    case LED_RED:
      pixels.setPixelColor(0, pixels.Color(127, 0, 0));
      break;
    case LED_GREEN:
      pixels.setPixelColor(0, pixels.Color(0, 127, 0));
      break;
    case LED_BLUE:
      pixels.setPixelColor(0, pixels.Color(0, 0, 127));
      break;
    case LED_OFF:
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      break;
  }
  pixels.show();
}
