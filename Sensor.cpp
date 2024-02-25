// Brightness Sensor
//
// February 9, 2024
// Tetsu Nishimura

#include <Arduino.h>
#include <Ticker.h>

#define PIN_LIGHT_SENSOR  A1

Ticker sensorTicker;

void sensorTimerHandler(void)
{
  int val;

  val = analogRead(PIN_LIGHT_SENSOR);
  Serial.println(val);
}

void sensorInit(void)
{
  sensorTicker.attach_ms(1000, sensorTimerHandler);
}