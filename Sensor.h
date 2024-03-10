// Brightness Sensor
//
// February 9, 2024
// Tetsu Nishimura
#ifndef SENSOR_H
#define SENSOR_H

#define STATE_BRIGHT  0x0001
#define STATE_DAYTIME 0x0002

typedef void(* stateChangedCallback)(uint16_t state);

void eventMonitorInit(stateChangedCallback cb);

#endif  // SENSOR_H
