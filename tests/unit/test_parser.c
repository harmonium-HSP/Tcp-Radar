#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include "../../src/parser.h"
#include "../../include/protocol.h"
#include <stdio.h>
#include <string.h>

static void test_parse_ethernet_header_valid(void **state) {
    (void) state;
    
    uint8_t packet[] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
        0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb,
        0x08, 0x00
    };
    
    struct ethernet_header eth;
    int result = parse_ethernet_header(packet, sizeof(packet), &eth);
    
    assert_int_equal(result, 0);
    assert_int_equal(eth.ethertype, ETHERTYPE_IP);
}

static void test_parse_ip_header_valid(void **state) {
    (void) state;
    
    uint8_t packet[] = {
        0x45, 0x00, 0x00, 0x3c, 0x1c, 0x46, 0x40, 0x00,
        0x40, 0x06, 0xb1, 0x9e, 0xc0, 0xa8, 0x01, 0x01,
        0x08, 0x08, 0x08, 0x08
    };
    
    struct ip_header ip;
    int result = parse_ip_header(packet, sizeof(packet), &ip);
    
    assert_int_equal(result, 0);
    assert_int_equal(ip.protocol, IP_PROTOCOL_TCP);
}

static void test_parse_tcp_header_valid(void **state) {
    (void) state;
    
    uint8_t packet[] = {
        0xc0, 0xa8, 0x01, 0x01, 0x00, 0x50, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00, 0x50, 0x02, 0x20, 0x00,
        0xb7, 0xd2, 0x00, 0x00
    };
    
    struct tcp_header tcp;
    int result = parse_tcp_header(packet, sizeof(packet), &tcp);
    
    assert_int_equal(result, 0);
    assert_int_equal(tcp.src_port, 0xa8c0);
}

static void test_parse_null_pointers(void **state) {
    (void) state;
    
    assert_int_equal(parse_ethernet_header(NULL, 0, NULL), -1);
    assert_int_equal(parse_ip_header(NULL, 0, NULL), -1);
    assert_int_equal(parse_tcp_header(NULL, 0, NULL), -1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_parse_ethernet_header_valid),
        cmocka_unit_test(test_parse_ip_header_valid),
        cmocka_unit_test(test_parse_tcp_header_valid),
        cmocka_unit_test(test_parse_null_pointers),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}