#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <stddef.h>
#include <stdint.h>

void ws_init(int port);
void ws_broadcast(const char *data, size_t len);
void ws_close(void);

#endif