#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>

#include "src/flow.h"
#include "src/parser.h"
#include "src/state_machine.h"
#include "src/congestion.h"
#include "src/capture.h"
#include "src/websocket.h"
#include "src/json.h"
#include "src/logger.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

static volatile int running = 1;

void signal_handler(int signum) {
    (void)signum;
    running = 0;
}

void print_usage(const char *prog_name) {
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  -i <interface>   Interface to capture on (e.g., eth0)\n");
    printf("  -p <port>        WebSocket port (default: 8080)\n");
    printf("  -h               Show this help\n");
}

int main(int argc, char *argv[]) {
    const char *interface = NULL;
    int ws_port = 8080;

    int opt;
    while ((opt = getopt(argc, argv, "i:p:h")) != -1) {
        switch (opt) {
            case 'i':
                interface = optarg;
                break;
            case 'p':
                ws_port = atoi(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    printf("TCP-Radar starting on ");
    if (interface) {
        printf("interface %s", interface);
    } else {
        printf("any interface");
    }
    printf(", WebSocket port %d\n", ws_port);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    log_init();
    flow_init();

    printf("TCP-Radar initialized successfully\n");

    while (running) {
        sleep(1);
    }

    log_close();
    printf("TCP-Radar shutting down\n");

    return 0;
}