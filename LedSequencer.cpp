// LED Sequencer
//
// April 5, 2024
// Tetsu Nishimura

#include <Arduino.h>
#include <Ticker.h>
#include "LedSequencer.h"

Ticker durationTicker;

void pixelCtrlHandler(void)
{
  durationTicker.detach();
  ledCtrlPixelOff();
}

void ledSeqSetPixel(pixel_state_t px, uint16_t duration)
{
  ledCtrlSetPixel(px);
  durationTicker.attach_ms(duration, pixelCtrlHandler);
}

void ledSeqSetPixelHue(uint16_t hue, uint16_t duration)
{
  ledCtrlSetPixelHue(hue);
  durationTicker.attach_ms(duration, pixelCtrlHandler);
}