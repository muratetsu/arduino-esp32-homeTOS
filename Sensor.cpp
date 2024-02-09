// Brightness Sensor
//
// February 9, 2024
// Tetsu Nishimura

#include <Arduino.h>
#include <Ticker.h>

Ticker sensorTicker;
const int sensorPin = A1;

void sensorTimerHandler(void)
{
  int val;

  val = analogRead(sensorPin);
  Serial.println(val);
}

void sensorInit(void)
{
  sensorTicker.attach_ms(1000, sensorTimerHandler);
}