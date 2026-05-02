#include "websocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

static int ws_server_socket = -1;

void ws_init(int port) {
    (void)port;
    printf("WebSocket server initialized on port %d\n", port);
}

void ws_broadcast(const char *data, size_t len) {
    (void)data;
    (void)len;
}

void ws_close(void) {
    if (ws_server_socket >= 0) {
#ifdef _WIN32
        closesocket(ws_server_socket);
#else
        close(ws_server_socket);
#endif
        ws_server_socket = -1;
    }
}