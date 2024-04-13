// Event Monitor
//
// February 9, 2024
// Tetsu Nishimura

#include <Arduino.h>
#include <Ticker.h>
#include "EventMonitor.h"
#include "daytime.h"

#define PIN_BRIGHTNESS_SENSOR A1
#define BRIGHTNESS_HIGH       150
#define BRIGHTNESS_LOW        80

Ticker eventMonitorTicker;
stateChangedCallback callback;
uint16_t currentState;
eventState_t eventState;

void eventMonitorGetStatus(eventState_t* state)
{
  state->min = eventState.min;
  state->max = eventState.max;
  eventState.min = 0xffff;
  eventState.max = 0x0000;
}

void eventMonitorTimerHandler(void)
{
  int brightness = analogRead(PIN_BRIGHTNESS_SENSOR);
  bool daytime = isDaytime();
  
  if (currentState & STATE_BRIGHT) {
    if (brightness < BRIGHTNESS_LOW) {
      currentState &= ~STATE_BRIGHT;
      callback(currentState);
    }
  }
  else {
    if (brightness > BRIGHTNESS_HIGH) {
      currentState |= STATE_BRIGHT;
      callback(currentState);
    }
  }

  if (currentState & STATE_DAYTIME) {
    if (!daytime) {
      currentState &= ~STATE_DAYTIME;
      callback(currentState);
    }
  }
  else {
    if (daytime) {
      currentState |= STATE_DAYTIME;
      callback(currentState);
    }
  }

  // store Min/Max value of brightness
  if (eventState.min > brightness) {
    eventState.min = brightness;
  }
  if (eventState.max < brightness) {
    eventState.max = brightness;
  }
}

void eventMonitorInit(stateChangedCallback cb)
{
  callback = cb;
  eventMonitorTicker.attach_ms(1000, eventMonitorTimerHandler);
}
