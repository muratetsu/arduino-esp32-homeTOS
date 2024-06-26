// LED Sequencer
//
// April 5, 2024
// Tetsu Nishimura
#ifndef LED_SEQUENCER_H
#define LED_SEQUENCER_H

#include "LedControl.h"

void ledSeqSetPixel(pixel_state_t px, uint16_t duration);
void ledSeqSetPixelHue(uint16_t hue, uint16_t duration);
void ledSeqBlinkPixel(uint16_t hue, uint16_t interval, uint16_t cycle);
void ledSeqSetLights(uint8_t val);

#endif  // LED_SEQUENCER_H
