// LED Control
//
// February 4, 2024
// Tetsu Nishimura
#ifndef LED_CONTROL_H
#define LED_CONTROL_H

typedef struct {
  uint16_t dulation;
  uint16_t hue;
  uint8_t sat;
  uint8_t val;
} pixel_state_t;

void ledCtrlInit(void);
void ledCtrlSetPixel(pixel_state_t px);

#endif  // LED_CONTROL_H
