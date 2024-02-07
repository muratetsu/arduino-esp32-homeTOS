// LED Control
//
// February 4, 2024
// Tetsu Nishimura
#ifndef LED_CONTROL_H
#define LED_CONTROL_H

void ledCtrlInit(void);
void ledCtrlSetPixel(int dulation, uint8_t red, uint8_t green, uint8_t blue);

#endif  // LED_CONTROL_H
