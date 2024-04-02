// OTA Firmware Update
//
// March 24, 2024
// Tetsu Nishimura
#ifndef OTA_H
#define OTA_H

#define HASH_SIZE               32
extern char firmwareHash[HASH_SIZE + 1];

void otaInit(void);

#endif  // OTA_H
