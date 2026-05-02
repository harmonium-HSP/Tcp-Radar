#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include "../../src/flow.h"
#include "../../include/tcp_states.h"
#include "../../include/congestion_types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint32_t test_hash_values[100];
static int hash_index = 0;

static void test_hash_consistency(void **state) {
    (void) state;
    
    flow_init();
    
    uint32_t h1 = flow_hash(0xC0A80001, 0xC0A80002, 12345, 80);
    uint32_t h2 = flow_hash(0xC0A80001, 0xC0A80002, 12345, 80);
    
    assert_int_equal(h1, h2);
    
    test_hash_values[hash_index++] = h1;
}

static void test_hash_different_keys(void **state) {
    (void) state;
    
    flow_init();
    
    uint32_t h1 = flow_hash(0xC0A80001, 0xC0A80002, 12345, 80);
    uint32_t h2 = flow_hash(0xC0A80001, 0xC0A80002, 12346, 80);
    uint32_t h3 = flow_hash(0xC0A80001, 0xC0A80002, 12345, 443);
    
    assert_true(h1 != h2 || h2 != h3);
}

static void test_insert_and_lookup(void **state) {
    (void) state;
    
    flow_init();
    
    struct tcp_flow *flow = flow_lookup_or_create(0xC0A80001, 0xC0A80002, 12345, 80);
    assert_non_null(flow);
    
    struct tcp_flow *found = flow_lookup(0xC0A80001, 0xC0A80002, 12345, 80);
    assert_non_null(found);
    assert_ptr_equal(found, flow);
}

static void test_same_key_returns_same_flow(void **state) {
    (void) state;
    
    flow_init();
    
    struct tcp_flow *flow1 = flow_lookup_or_create(0xC0A80001, 0xC0A80002, 12345, 80);
    struct tcp_flow *flow2 = flow_lookup_or_create(0xC0A80001, 0xC0A80002, 12345, 80);
    
    assert_ptr_equal(flow1, flow2);
}

static void test_update_last_activity(void **state) {
    (void) state;
    
    flow_init();
    flow_table_set_current_time(1000);
    
    struct tcp_flow *flow = flow_lookup_or_create(0xC0A80001, 0xC0A80002, 12345, 80);
    assert_non_null(flow);
    assert_int_equal(flow->last_activity, 1000);
    
    flow_table_set_current_time(2000);
    
    struct tcp_flow *found = flow_lookup_or_create(0xC0A80001, 0xC0A80002, 12345, 80);
    assert_non_null(found);
    assert_ptr_equal(found, flow);
    assert_int_equal(found->last_activity, 2000);
}

static void test_timeout_cleanup(void **state) {
    (void) state;
    
    flow_init();
    flow_table_set_current_time(1000);
    
    flow_lookup_or_create(0xC0A80001, 0xC0A80002, 12345, 80);
    
    flow_table_set_current_time(2000);
    flow_lookup_or_create(0xC0A80001, 0xC0A80002, 12346, 80);
    
    flow_cleanup_timeout(3000, 500);
    
    assert_null(flow_lookup(0xC0A80001, 0xC0A80002, 12345, 80));
    assert_non_null(flow_lookup(0xC0A80001, 0xC0A80002, 12346, 80));
}

static void test_hash_collision(void **state) {
    (void) state;
    
    flow_init();
    
    struct tcp_flow *flow1 = flow_lookup_or_create(0xC0A80001, 0xC0A80002, 12345, 80);
    struct tcp_flow *flow2 = flow_lookup_or_create(0xC0A80001, 0xC0A80002, 12346, 80);
    
    assert_true(flow1 != flow2);
    
    assert_non_null(flow_lookup(0xC0A80001, 0xC0A80002, 12345, 80));
    assert_non_null(flow_lookup(0xC0A80001, 0xC0A80002, 12346, 80));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_hash_consistency),
        cmocka_unit_test(test_hash_different_keys),
        cmocka_unit_test(test_insert_and_lookup),
        cmocka_unit_test(test_same_key_returns_same_flow),
        cmocka_unit_test(test_update_last_activity),
        cmocka_unit_test(test_timeout_cleanup),
        cmocka_unit_test(test_hash_collision),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}