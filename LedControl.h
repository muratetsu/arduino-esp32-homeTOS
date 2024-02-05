// LED Control
//
// February 4, 2024
// Tetsu Nishimura
#ifndef LED_CONTROL_H
#define LED_CONTROL_H

typedef enum {
  LED_RED,
  LED_GREEN,
  LED_BLUE,
  LED_OFF,
} pixel_state_t;

void ledCtrlInit(void);
void ledCtrlStartTimer(void);
void ledCtrlSetPixel(pixel_state_t state);

#endif  // LED_CONTROL_H
