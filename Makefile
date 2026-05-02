CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude
LDFLAGS =

SRC_DIR = src
BUILD_DIR = build
TARGET = $(BUILD_DIR)/tcp-radar

SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/capture.c $(SRC_DIR)/parser.c $(SRC_DIR)/flow.c $(SRC_DIR)/state_machine.c $(SRC_DIR)/state_helper.c $(SRC_DIR)/congestion.c $(SRC_DIR)/websocket.c $(SRC_DIR)/json.c $(SRC_DIR)/pcap_export.c $(SRC_DIR)/logger.c
OBJECTS = $(SOURCES:.c=.o)
OBJECTS := $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(OBJECTS))

LIBS = -lpthread -lssl -lcrypto -lsqlite3 -lcap-ng

TEST_DIR = tests/unit
TEST_SRC = $(wildcard $(TEST_DIR)/test_*.c)
TEST_BINS = $(TEST_SRC:.c=)
TEST_CFLAGS = -Wall -Wextra -O2 -Iinclude -DUNIT_TESTING
TEST_LDFLAGS = -lcmocka -lpthread

.PHONY: all clean test test-build test-clean

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

test-build: $(TEST_BINS)

$(TEST_DIR)/test_parser: $(TEST_DIR)/test_parser.c $(SRC_DIR)/parser.c $(SRC_DIR)/parser.h
	$(CC) $(TEST_CFLAGS) $< $(SRC_DIR)/parser.c $(TEST_LDFLAGS) -o $@

$(TEST_DIR)/test_flow: $(TEST_DIR)/test_flow.c $(SRC_DIR)/flow.c $(SRC_DIR)/state_helper.c $(SRC_DIR)/flow.h
	$(CC) $(TEST_CFLAGS) -Iinclude $< $(SRC_DIR)/flow.c $(SRC_DIR)/state_helper.c $(TEST_LDFLAGS) -o $@

$(TEST_DIR)/test_state_machine: $(TEST_DIR)/test_state_machine.c $(SRC_DIR)/state_machine.c $(SRC_DIR)/state_helper.c $(SRC_DIR)/flow.c $(SRC_DIR)/logger_stub.c
	$(CC) $(TEST_CFLAGS) -Iinclude $< $(SRC_DIR)/state_machine.c $(SRC_DIR)/state_helper.c $(SRC_DIR)/flow.c $(SRC_DIR)/logger_stub.c $(TEST_LDFLAGS) -o $@

$(TEST_DIR)/test_congestion: $(TEST_DIR)/test_congestion.c $(SRC_DIR)/congestion.c $(SRC_DIR)/flow.c $(SRC_DIR)/state_helper.c $(SRC_DIR)/congestion_stub.c
	$(CC) $(TEST_CFLAGS) -Iinclude $< $(SRC_DIR)/congestion.c $(SRC_DIR)/flow.c $(SRC_DIR)/state_helper.c $(SRC_DIR)/congestion_stub.c $(TEST_LDFLAGS) -o $@

$(TEST_DIR)/test_json: $(TEST_DIR)/test_json.c $(SRC_DIR)/json.c $(SRC_DIR)/flow.c $(SRC_DIR)/state_helper.c
	$(CC) $(TEST_CFLAGS) -Iinclude $< $(SRC_DIR)/json.c $(SRC_DIR)/flow.c $(SRC_DIR)/state_helper.c $(TEST_LDFLAGS) -o $@

$(TEST_DIR)/test_logger: $(TEST_DIR)/test_logger.c $(SRC_DIR)/logger.c
	$(CC) $(TEST_CFLAGS) -Iinclude $< $(SRC_DIR)/logger.c $(TEST_LDFLAGS) -lsqlite3 -pthread -o $@

$(TEST_DIR)/test_%: $(TEST_DIR)/test_%.c
	$(CC) $(TEST_CFLAGS) $< $(TEST_LDFLAGS) -o $@

test-clean:
	rm -f $(TEST_BINS)

test: test-build
	./tests/run_tests.sh

# Docker 相关
docker-build:
	docker build -t tcp-radar:latest .

docker-run:
	docker run --rm \
		--name tcp-radar \
		--network host \
		--cap-add NET_ADMIN \
		--cap-add NET_RAW \
		-v $(PWD)/exports:/app/exports \
		tcp-radar:latest -i eth0 -p 8080

docker-compose-up:
	docker-compose up -d

docker-compose-down:
	docker-compose down

docker-logs:
	docker logs -f tcp-radar

docker-shell:
	docker exec -it tcp-radar /bin/sh