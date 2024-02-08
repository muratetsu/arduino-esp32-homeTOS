// LED Control
//
// February 4, 2024
// Tetsu Nishimura
#ifndef LED_CONTROL_H
#define LED_CONTROL_H

typedef struct {
  int dulation;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} pixel_state_t;

void ledCtrlInit(void);
void ledCtrlSetPixel(pixel_state_t px);

#endif  // LED_CONTROL_H
