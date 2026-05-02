#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/time.h>
#endif

static FILE *log_file = NULL;

void log_init(void) {
    log_file = fopen("tcp-radar.log", "a");
    if (log_file) {
        fprintf(log_file, "\n--- TCP-Radar started ---\n");
        fflush(log_file);
    }
}

void log_event(const char *event_type, const char *flow_key,
               const char *details, uint32_t cwnd, int rtt_ms) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    FILE *fp = log_file ? log_file : stdout;
    fprintf(fp, "[%s] %s | %s | %s | cwnd=%u rtt=%d\n",
            time_str, event_type, flow_key, details, cwnd, rtt_ms);
    fflush(fp);
}

void log_close(void) {
    if (log_file) {
        fprintf(log_file, "--- TCP-Radar stopped ---\n\n");
        fclose(log_file);
        log_file = NULL;
    }
}