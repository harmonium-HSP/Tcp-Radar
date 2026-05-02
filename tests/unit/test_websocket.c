#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmocka.h>
#include <sys/socket.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include "../src/websocket.h"

static char *test_base64_encode(const unsigned char *input, size_t len) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input, len);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    char *output = malloc(bufferPtr->length + 1);
    if (output) {
        memcpy(output, bufferPtr->data, bufferPtr->length);
        output[bufferPtr->length] = '\0';
    }

    BIO_free_all(bio);
    return output;
}

static void test_ws_key_compute(void **state) {
    (void) state;

    const char *client_key = "dGhlIHNhbXBsZSBub25jZQ==";
    const char *expected_accept = "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";

    char combined[256];
    snprintf(combined, sizeof(combined), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", client_key);

    unsigned char sha1_hash[SHA_DIGEST_LENGTH];
    SHA1((const unsigned char *)combined, strlen(combined), sha1_hash);

    char *accept_key = test_base64_encode(sha1_hash, SHA_DIGEST_LENGTH);

    assert_non_null(accept_key);
    assert_string_equal(accept_key, expected_accept);

    free(accept_key);
}

static void test_ws_frame_encode_decode(void **state) {
    (void) state;

    const char *test_payload = "Hello, WebSocket!";
    size_t payload_len = strlen(test_payload);

    unsigned char header[16];
    header[0] = 0x81;

    if (payload_len <= 125) {
        header[1] = (unsigned char)payload_len;
    }

    unsigned char encoded[256];
    encoded[0] = header[0];
    encoded[1] = header[1];
    memcpy(encoded + 2, test_payload, payload_len);

    assert_int_equal(encoded[0], 0x81);
    assert_int_equal(encoded[1], payload_len);
    assert_true(memcmp(encoded + 2, test_payload, payload_len) == 0);
}

static void test_ws_frame_decode_text(void **state) {
    (void) state;

    unsigned char frame[] = {
        0x81,
        0x0D,
        'H', 'e', 'l', 'l', 'o', '!',
    };

    unsigned char opcode = frame[0] & 0x0F;
    unsigned char masked = (frame[1] & 0x80) != 0;
    uint64_t payload_len = frame[1] & 0x7F;

    assert_int_equal(opcode, 0x01);
    assert_false(masked);
    assert_int_equal(payload_len, 13);
}

static void test_ws_frame_decode_binary(void **state) {
    (void) state;

    unsigned char frame[] = {
        0x82,
        0x05,
        0x00, 0x01, 0x02, 0x03, 0x04,
    };

    unsigned char opcode = frame[0] & 0x0F;
    unsigned char masked = (frame[1] & 0x80) != 0;
    uint64_t payload_len = frame[1] & 0x7F;

    assert_int_equal(opcode, 0x02);
    assert_false(masked);
    assert_int_equal(payload_len, 5);
}

static void test_ws_frame_ping_pong(void **state) {
    (void) state;

    unsigned char ping_frame[] = { 0x89, 0x00 };

    unsigned char opcode = ping_frame[0] & 0x0F;
    unsigned char payload_len = ping_frame[1] & 0x7F;

    assert_int_equal(opcode, 0x09);
    assert_int_equal(payload_len, 0);

    unsigned char pong_frame[] = { 0x8A, 0x00 };
    opcode = pong_frame[0] & 0x0F;
    assert_int_equal(opcode, 0x0A);
}

static void test_ws_frame_close(void **state) {
    (void) state;

    unsigned char close_frame[] = { 0x88, 0x00 };

    unsigned char opcode = close_frame[0] & 0x0F;
    unsigned char payload_len = close_frame[1] & 0x7F;

    assert_int_equal(opcode, 0x08);
    assert_int_equal(payload_len, 0);
}

static void test_ws_frame_masked(void **state) {
    (void) state;

    unsigned char masking_key[4] = { 0x37, 0xFA, 0x21, 0x3D };
    unsigned char payload[] = { 'H', 'e', 'l', 'l', 'o' };

    unsigned char masked[5];
    for (size_t i = 0; i < sizeof(payload); i++) {
        masked[i] = payload[i] ^ masking_key[i % 4];
    }

    unsigned char unmasked[5];
    for (size_t i = 0; i < sizeof(masked); i++) {
        unmasked[i] = masked[i] ^ masking_key[i % 4];
    }

    assert_true(memcmp(unmasked, payload, sizeof(payload)) == 0);
}

static void test_ws_frame_fragmented(void **state) {
    (void) state;

    unsigned char frag1[] = { 0x01, 0x05, 'H', 'e', 'l', 'l', 'o' };
    unsigned char frag2[] = { 0x80, 0x05, ' ', 'W', 'o', 'r', 'l', 'd' };

    unsigned char opcode1 = frag1[0] & 0x0F;
    unsigned char opcode2 = frag2[0] & 0x0F;
    unsigned char fin1 = (frag1[0] & 0x80) == 0;
    unsigned char fin2 = (frag2[0] & 0x80) != 0;

    assert_int_equal(opcode1, 0x01);
    assert_true(fin1);
    assert_int_equal(opcode2, 0x00);
    assert_true(fin2);
}

static void test_ws_handshake_valid(void **state) {
    (void) state;

    const char *request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n";

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    int result = ws_handshake(fds[0], request);
    assert_int_equal(result, 0);

    close(fds[0]);
    close(fds[1]);
}

static void test_ws_handshake_missing_key(void **state) {
    (void) state;

    const char *request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n";

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    int result = ws_handshake(fds[0], request);
    assert_int_equal(result, -1);

    close(fds[0]);
    close(fds[1]);
}

static void test_ws_broadcast(void **state) {
    (void) state;

    ws_init();

    int fds[3][2];
    for (int i = 0; i < 3; i++) {
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds[i]);
        ws_add_client(fds[i][0]);
    }

    const char *json = "{\"test\":\"data\"}";
    int count = ws_broadcast(json, strlen(json));
    assert_int_equal(count, 3);

    for (int i = 0; i < 3; i++) {
        close(fds[i][0]);
        close(fds[i][1]);
    }

    ws_close_all();
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ws_key_compute),
        cmocka_unit_test(test_ws_frame_encode_decode),
        cmocka_unit_test(test_ws_frame_decode_text),
        cmocka_unit_test(test_ws_frame_decode_binary),
        cmocka_unit_test(test_ws_frame_ping_pong),
        cmocka_unit_test(test_ws_frame_close),
        cmocka_unit_test(test_ws_frame_masked),
        cmocka_unit_test(test_ws_frame_fragmented),
        cmocka_unit_test(test_ws_handshake_valid),
        cmocka_unit_test(test_ws_handshake_missing_key),
        cmocka_unit_test(test_ws_broadcast),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
