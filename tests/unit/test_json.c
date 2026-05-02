#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <cmocka.h>
#include "../../src/flow.h"
#include "../../src/json.h"
#include "../../include/tcp_states.h"
#include "../../include/congestion_types.h"

static int is_valid_json_object(const char *str) {
    if (!str || str[0] != '{') return 0;
    int len = strlen(str);
    if (len < 2 || str[len-1] != '}') return 0;
    return 1;
}

static int json_contains_key(const char *json, const char *key) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    return strstr(json, pattern) != NULL;
}

static int json_contains_value(const char *json, const char *value) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", value);
    return strstr(json, pattern) != NULL;
}

static void test_serialize_flow_metrics_basic(void **state) {
    (void)state;

    struct tcp_flow *flow = flow_lookup_or_create(0xC0A80101, 0xC0A80102, 12345, 80);
    assert_non_null(flow);

    flow->cc.inflight_bytes = 1000;
    flow->cc.cwnd_estimate = 5000;
    flow->cc.rtt_ms = 50;
    flow->cc.retrans_count = 2;

    char buf[1024];
    serialize_flow_metrics(buf, sizeof(buf), flow, NULL, 0);

    assert_true(is_valid_json_object(buf));
    assert_true(json_contains_key(buf, "flow_id"));
    assert_true(json_contains_key(buf, "timestamp"));
    assert_true(json_contains_key(buf, "state"));
    assert_true(json_contains_key(buf, "inflight_bytes"));
    assert_true(json_contains_key(buf, "cwnd_estimate"));
    assert_true(json_contains_key(buf, "rtt_ms"));
    assert_true(json_contains_key(buf, "retrans_count"));
}

static void test_serialize_flow_metrics_null_flow(void **state) {
    (void)state;

    char buf[1024];
    serialize_flow_metrics(buf, sizeof(buf), NULL, NULL, 0);

    assert_string_equal(buf, "{}");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_serialize_flow_metrics_basic),
        cmocka_unit_test(test_serialize_flow_metrics_null_flow),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}