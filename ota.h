// HTTP Update
//
// March 24, 2024
// Tetsu Nishimura
#ifndef HTTP_UPDATE_H
#define HTTP_UPDATE_H

#define HASH_SIZE               32
extern char firmwareHash[HASH_SIZE + 1];

void httpUpdateInit(void);

#endif  // HTTP_UPDATE_H
