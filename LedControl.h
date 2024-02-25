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

typedef struct {
  uint8_t street;
  uint8_t entrance;
  uint8_t room;
} led_val_t;

void ledCtrlInit(void);
void ledCtrlSetPixel(pixel_state_t px);
void ledCtrlSetPixelRed(void);
void ledCtrlSetPixelOff(void);
void ledCtrlSetStreetLight(uint8_t val);
void ledCtrlSetEntranceLight(uint8_t val);
void ledCtrlSetRoomLight(uint8_t val);

#endif  // LED_CONTROL_H
