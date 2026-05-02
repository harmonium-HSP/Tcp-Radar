#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <cmocka.h>
#include <sqlite3.h>
#include <unistd.h>

#include "../include/logger.h"

#define TEST_DB_PATH ":memory:"

static int get_event_count(const char *db_path) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int count = 0;

    if (sqlite3_open(db_path, &db) == SQLITE_OK) {
        if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM events", -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                count = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
        sqlite3_close(db);
    }

    return count;
}

static void wait_for_async_queue(void) {
    usleep(500000);
}

static void test_logger_init(void **state) {
    (void) state;

    int result = logger_init(TEST_DB_PATH);
    assert_int_equal(result, 0);

    logger_shutdown();
}

static void test_log_event_insert(void **state) {
    (void) state;

    int result = logger_init(TEST_DB_PATH);
    assert_int_equal(result, 0);

    result = log_event("TEST_EVENT", "flow1", "test details", 1000, 50);
    assert_int_equal(result, 0);

    wait_for_async_queue();
    assert_true(get_event_count(TEST_DB_PATH) > 0);

    logger_shutdown();
}

static void test_log_event_multiple(void **state) {
    (void) state;

    int result = logger_init(TEST_DB_PATH);
    assert_int_equal(result, 0);

    for (int i = 0; i < 10; i++) {
        result = log_event("TEST_EVENT", "flow1", "test details", 1000, 50);
        assert_int_equal(result, 0);
    }

    wait_for_async_queue();
    assert_int_equal(get_event_count(TEST_DB_PATH), 10);

    logger_shutdown();
}

static void test_logger_init_twice(void **state) {
    (void) state;

    int result = logger_init(TEST_DB_PATH);
    assert_int_equal(result, 0);

    result = logger_init(TEST_DB_PATH);
    assert_int_equal(result, 0);

    logger_shutdown();
}

static void test_logger_invalid_path(void **state) {
    (void) state;

    int result = logger_init("/nonexistent/path/test.db");
    assert_int_not_equal(result, 0);
}

static void test_logger_close(void **state) {
    (void) state;

    int result = logger_init(TEST_DB_PATH);
    assert_int_equal(result, 0);

    for (int i = 0; i < 5; i++) {
        result = log_event("TEST_EVENT", "flow1", "test details", 1000, 50);
        assert_int_equal(result, 0);
    }

    logger_shutdown();
}

static void test_logger_null_event_type(void **state) {
    (void) state;

    int result = logger_init(TEST_DB_PATH);
    assert_int_equal(result, 0);

    result = log_event(NULL, "flow1", "test details", 1000, 50);
    assert_int_not_equal(result, 0);

    logger_shutdown();
}

static void test_logger_empty_event_type(void **state) {
    (void) state;

    int result = logger_init(TEST_DB_PATH);
    assert_int_equal(result, 0);

    result = log_event("", "flow1", "test details", 1000, 50);
    assert_int_equal(result, 0);

    logger_shutdown();
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_logger_init),
        cmocka_unit_test(test_log_event_insert),
        cmocka_unit_test(test_log_event_multiple),
        cmocka_unit_test(test_logger_init_twice),
        cmocka_unit_test(test_logger_invalid_path),
        cmocka_unit_test(test_logger_close),
        cmocka_unit_test(test_logger_null_event_type),
        cmocka_unit_test(test_logger_empty_event_type),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}