#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include "../../src/state_machine.h"
#include "../../src/flow.h"
#include "../../include/tcp_states.h"

static void test_closed_to_syn_sent(void **state) {
    (void) state;
    assert_int_equal(TCP_STATE_CLOSED, TCP_STATE_CLOSED);
}

static void test_closed_to_syn_rcvd(void **state) {
    (void) state;
    assert_int_equal(TCP_STATE_CLOSED, TCP_STATE_CLOSED);
}

static void test_closed_rst(void **state) {
    (void) state;
    assert_int_equal(TCP_STATE_CLOSED, TCP_STATE_CLOSED);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_closed_to_syn_sent),
        cmocka_unit_test(test_closed_to_syn_rcvd),
        cmocka_unit_test(test_closed_rst),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}