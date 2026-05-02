#include "capture.h"
#include "parser.h"
#include "flow.h"
#include "state_machine.h"
#include "congestion.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <pcap.h>
#include <netinet/in.h>
#endif

static packet_callback_t g_callback = NULL;

#ifdef _WIN32
static pcap_t *g_pcap = NULL;
#else
static pcap_t *g_pcap = NULL;
#endif

bool capture_init(const char *interface, packet_callback_t callback) {
    (void)interface;
    g_callback = callback;
    return true;
}

bool capture_start(void) {
    return true;
}

bool capture_stop(void) {
    if (g_pcap) {
#ifdef _WIN32
        pcap_close(g_pcap);
#else
        pcap_close(g_pcap);
#endif
        g_pcap = NULL;
    }
    return true;
}