#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmocka.h>
#include <sys/stat.h>
#include <stdio.h>

#include "../src/pcap_export.h"

static char g_temp_pcap[256] = "/tmp/test_export.pcap";

static int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static size_t file_size(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return st.st_size;
}

static void test_pcap_init(void **state) {
    (void) state;

    unlink(g_temp_pcap);
    int result = pcap_export_init(g_temp_pcap);
    assert_int_equal(result, 0);
    assert_true(file_exists(g_temp_pcap));

    size_t sz = file_size(g_temp_pcap);
    assert_true(sz >= 24);

    pcap_export_close();
}

static void test_pcap_write_packet(void **state) {
    (void) state;

    unlink(g_temp_pcap);
    int result = pcap_export_init(g_temp_pcap);
    assert_int_equal(result, 0);

    unsigned char packet[14] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x08, 0x00};

    pcap_export_add_packet(packet, sizeof(packet));

    size_t sz = file_size(g_temp_pcap);
    assert_true(sz >= 24 + 16 + sizeof(packet));

    pcap_export_close();
}

static void test_pcap_multiple_packets(void **state) {
    (void) state;

    unlink(g_temp_pcap);
    int result = pcap_export_init(g_temp_pcap);
    assert_int_equal(result, 0);

    unsigned char packet1[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                               0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
                               0x08, 0x00};
    unsigned char packet2[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
                               0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb,
                               0x08, 0x00};

    pcap_export_add_packet(packet1, sizeof(packet1));
    pcap_export_add_packet(packet2, sizeof(packet2));

    size_t sz = file_size(g_temp_pcap);
    assert_true(sz >= 24 + 2 * (16 + sizeof(packet1)));

    pcap_export_close();
}

static void test_pcap_file_header(void **state) {
    (void) state;

    unlink(g_temp_pcap);
    int result = pcap_export_init(g_temp_pcap);
    assert_int_equal(result, 0);

    FILE *fp = fopen(g_temp_pcap, "rb");
    assert_non_null(fp);

    pcap_file_header_t header;
    size_t read = fread(&header, sizeof(header), 1, fp);
    assert_int_equal(read, 1);

    assert_int_equal(header.magic_number, 0xa1b2c3d4);
    assert_int_equal(header.version_major, 2);
    assert_int_equal(header.version_minor, 4);
    assert_int_equal(header.snaplen, 65535);
    assert_int_equal(header.network, 1);

    fclose(fp);
    pcap_export_close();
}

static void test_pcap_packet_header(void **state) {
    (void) state;

    unlink(g_temp_pcap);
    int result = pcap_export_init(g_temp_pcap);
    assert_int_equal(result, 0);

    unsigned char packet[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x08, 0x00};

    pcap_export_add_packet(packet, sizeof(packet));

    FILE *fp = fopen(g_temp_pcap, "rb");
    assert_non_null(fp);

    fseek(fp, 24, SEEK_SET);

    pcap_pkthdr_t pkt_hdr;
    fread(&pkt_hdr, sizeof(pkt_hdr), 1, fp);

    assert_int_equal(pkt_hdr.incl_len, sizeof(packet));
    assert_int_equal(pkt_hdr.orig_len, sizeof(packet));

    fclose(fp);
    pcap_export_close();
}

static void test_pcap_invalid_filename(void **state) {
    (void) state;

    unlink("/nonexistent/path/test.pcap");
    int result = pcap_export_init("/nonexistent/path/test.pcap");
    assert_int_not_equal(result, 0);
}

static void test_pcap_close(void **state) {
    (void) state;

    unlink(g_temp_pcap);
    int result = pcap_export_init(g_temp_pcap);
    assert_int_equal(result, 0);

    unsigned char packet[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x08, 0x00};

    pcap_export_add_packet(packet, sizeof(packet));
    pcap_export_close();

    size_t sz = file_size(g_temp_pcap);
    assert_true(sz >= 24 + 16 + sizeof(packet));
}

static void test_pcap_large_packet(void **state) {
    (void) state;

    unlink(g_temp_pcap);
    int result = pcap_export_init(g_temp_pcap);
    assert_int_equal(result, 0);

    unsigned char *large_packet = malloc(1500);
    assert_non_null(large_packet);
    memset(large_packet, 0xAA, 1500);

    pcap_export_add_packet(large_packet, 1500);

    size_t sz = file_size(g_temp_pcap);
    assert_true(sz >= 24 + 16 + 1500);

    free(large_packet);
    pcap_export_close();
}

static void test_pcap_empty_packet(void **state) {
    (void) state;

    unlink(g_temp_pcap);
    int result = pcap_export_init(g_temp_pcap);
    assert_int_equal(result, 0);

    unsigned char packet[1] = {0x00};

    pcap_export_add_packet(packet, 0);

    FILE *fp = fopen(g_temp_pcap, "rb");
    assert_non_null(fp);

    fseek(fp, 24, SEEK_SET);

    pcap_pkthdr_t pkt_hdr;
    size_t read = fread(&pkt_hdr, sizeof(pkt_hdr), 1, fp);
    assert_int_equal(read, 1);

    assert_int_equal(pkt_hdr.incl_len, 0);
    assert_int_equal(pkt_hdr.orig_len, 0);

    fclose(fp);
    pcap_export_close();
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pcap_init),
        cmocka_unit_test(test_pcap_write_packet),
        cmocka_unit_test(test_pcap_multiple_packets),
        cmocka_unit_test(test_pcap_file_header),
        cmocka_unit_test(test_pcap_packet_header),
        cmocka_unit_test(test_pcap_invalid_filename),
        cmocka_unit_test(test_pcap_close),
        cmocka_unit_test(test_pcap_large_packet),
        cmocka_unit_test(test_pcap_empty_packet),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
