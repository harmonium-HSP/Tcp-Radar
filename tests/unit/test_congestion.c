#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include "../../src/congestion.h"
#include "../../src/flow.h"
#include "../../include/congestion_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct tcp_flow* create_test_flow(void) {
    flow_init();
    struct tcp_flow *flow = flow_lookup_or_create(0xC0A80101, 0xC0A80102, 12345, 80);
    return flow;
}

static void free_test_flow(struct tcp_flow *flow) {
    (void)flow;
}

static void test_cc_init(void **state) {
    (void) state;
    struct tcp_flow *flow = create_test_flow();
    cc_init_flow(flow);
    assert_int_equal(flow->cc.rto_ms, 1000);
    assert_int_equal(flow->cc.ssthresh_estimate, 0xFFFF);
    free_test_flow(flow);
}

static void test_cwnd_update_initial(void **state) {
    (void) state;
    struct tcp_flow *flow = create_test_flow();
    cc_init_flow(flow);
    cc_update_cwnd_estimate(flow);
    assert_int_equal(flow->cc.cwnd_estimate, MSS * 2);
    free_test_flow(flow);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_cc_init),
        cmocka_unit_test(test_cwnd_update_initial),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}