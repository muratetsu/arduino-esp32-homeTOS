// LED Sequencer
//
// April 5, 2024
// Tetsu Nishimura

#include <Arduino.h>
#include <Ticker.h>
#include "LedSequencer.h"

// Duration between first light control and second light control
#define LIGHT_CTRL_DURATION 2000

Ticker pixelTicker;
Ticker lightTicker;

//*******************************************************
// Pixel Control Functions

void pixelCtrlHandler(void)
{
  pixelTicker.detach();
  ledCtrlPixelOff();
}

void ledSeqSetPixel(pixel_state_t px, uint16_t duration)
{
  ledCtrlSetPixel(px);
  pixelTicker.attach_ms(duration, pixelCtrlHandler);
}

void ledSeqSetPixelHue(uint16_t hue, uint16_t duration)
{
  ledCtrlSetPixelHue(hue);
  pixelTicker.attach_ms(duration, pixelCtrlHandler);
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
