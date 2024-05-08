// LED Sequencer
//
// April 5, 2024
// Tetsu Nishimura

#include <Arduino.h>
#include <Ticker.h>
#include "LedSequencer.h"

// Duration between first light control and second light control
#define LIGHT_CTRL_DURATION 4000

Ticker pixelTicker;
Ticker lightTicker;

uint16_t blinkHue;
int32_t blinkCycle;

//*******************************************************
// Pixel Control Functions

void pixelCtrlHandler(void)
{
  if (blinkCycle & 0x0001) {
    ledCtrlSetPixelHue(blinkHue);
  }
  else {
    ledCtrlPixelOff();
  }

  if (blinkCycle <= 0) {
    pixelTicker.detach();
  }

  blinkCycle--;
}

void ledSeqSetPixel(pixel_state_t px, uint16_t duration)
{
  blinkCycle = 0;
  ledCtrlSetPixel(px);
  pixelTicker.attach_ms(duration, pixelCtrlHandler);
}

void ledSeqSetPixelHue(uint16_t hue, uint16_t duration)
{
  blinkCycle = 0;
  ledCtrlSetPixelHue(hue);
  pixelTicker.attach_ms(duration, pixelCtrlHandler);
}

void ledSeqBlinkPixel(uint16_t hue, uint16_t interval, uint16_t cycle)
{
  blinkHue = hue;
  blinkCycle = (cycle - 1) * 2;
  ledCtrlSetPixelHue(hue);
  pixelTicker.attach_ms(interval / 2, pixelCtrlHandler);
}

//*******************************************************
// Light Control Functions

void lightCtrlHandler(uint8_t val)
{
  lightTicker.detach();
  ledCtrlSetRoomLight(val);
}

void ledSeqSetLights(uint8_t val)
{
  ledCtrlSetEntranceLight(val);
  lightTicker.attach_ms(2000, lightCtrlHandler, val);
}
