#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define ANSI_COLOR_RESET   "\033[0m"
#define ANSI_COLOR_RED      "\033[31m"
#define ANSI_COLOR_GREEN    "\033[32m"
#define ANSI_COLOR_YELLOW   "\033[33m"
#define ANSI_COLOR_BLUE     "\033[34m"
#define ANSI_COLOR_MAGENTA  "\033[35m"
#define ANSI_COLOR_CYAN     "\033[36m"

void print_banner(void) {
    printf("\n");
    printf(ANSI_COLOR_CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" ANSI_COLOR_RESET);
    printf(ANSI_COLOR_CYAN "  " ANSI_COLOR_RESET ANSI_COLOR_YELLOW "TCP-Radar Unit Tests" ANSI_COLOR_RESET ANSI_COLOR_CYAN "                              
" ANSI_COLOR_RESET);
    printf(ANSI_COLOR_CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n" ANSI_COLOR_RESET);
}

void print_summary(int suites_passed, int suites_failed, int cases_passed, int cases_failed, double elapsed) {
    printf("\n");
    printf(ANSI_COLOR_CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" ANSI_COLOR_RESET);
    printf(ANSI_COLOR_YELLOW "📊 Summary\n" ANSI_COLOR_RESET);
    printf(ANSI_COLOR_CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" ANSI_COLOR_RESET);
    printf("  Test suites: ");
    if (suites_passed > 0) printf(ANSI_COLOR_GREEN "%d passed" ANSI_COLOR_RESET, suites_passed);
    if (suites_failed > 0) printf(", " ANSI_COLOR_RED "%d failed" ANSI_COLOR_RESET, suites_failed);
    printf("\n");
    printf("  Test cases:  ");
    if (cases_passed > 0) printf(ANSI_COLOR_GREEN "%d passed" ANSI_COLOR_RESET, cases_passed);
    if (cases_failed > 0) printf(", " ANSI_COLOR_RED "%d failed" ANSI_COLOR_RESET, cases_failed);
    printf("\n");
    printf("  Time:        %.3fs\n", elapsed);
    printf(ANSI_COLOR_CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" ANSI_COLOR_RESET);

    if (cases_failed > 0) {
        printf(ANSI_COLOR_RED "  ❌ SOME TESTS FAILED!\n" ANSI_COLOR_RESET);
    } else {
        printf(ANSI_COLOR_GREEN "  ✅ ALL TESTS PASSED!\n" ANSI_COLOR_RESET);
    }
    printf(ANSI_COLOR_CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n" ANSI_COLOR_RESET);
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    print_banner();

    system("make clean 2>/dev/null");
    printf(ANSI_COLOR_BLUE "🔨 Compiling tests...\n\n" ANSI_COLOR_RESET);

    int compile_result = system("make test-build 2>&1");
    if (compile_result != 0) {
        printf(ANSI_COLOR_RED "\n❌ Compilation failed!\n\n" ANSI_COLOR_RESET);
        return 1;
    }

    printf("\n" ANSI_COLOR_BLUE "🧪 Running test suites...\n\n" ANSI_COLOR_RESET);

    clock_t start = clock();

    int total_passed = 0;
    int total_failed = 0;
    int suites_passed = 0;
    int suites_failed = 0;

    FILE *fp = popen("find tests/unit -maxdepth 1 -type f -executable -name 'test_*' ! -name '*.c' 2>/dev/null | sort", "r");
    if (fp) {
        char path[256];
        while (fgets(path, sizeof(path), fp) != NULL) {
            path[strcspn(path, "\n")] = 0;
            if (strstr(path, ".c")) continue;

            char cmd[512];
            snprintf(cmd, sizeof(cmd), "%s 2>&1", path);
            int result = system(cmd);

            if (result == 0) {
                total_passed++;
            } else {
                total_failed++;
            }
        }
        pclose(fp);
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    suites_passed = (total_failed == 0) ? 1 : 0;
    suites_failed = (total_failed > 0) ? 1 : 0;

    print_summary(suites_passed, suites_failed, total_passed, total_failed, elapsed);

    return (total_failed > 0) ? 1 : 0;
}