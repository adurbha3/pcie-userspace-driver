CC := gcc
# -std=c11 alone hides POSIX declarations (ftruncate, shm_open, etc.) behind
# glibc's feature-test macros; -D_POSIX_C_SOURCE opts back into them while
# keeping strict C11 syntax checking.
CFLAGS := -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -Iinclude -g
LDFLAGS := -lrt

BUILD := build

CORE_SRCS := src/device_model.c src/mmio.c src/ring_buffer.c src/netproto.c
CORE_OBJS := $(patsubst src/%.c,$(BUILD)/%.o,$(CORE_SRCS))

.PHONY: all test integration-test clean

all: $(BUILD)/demo_driver $(BUILD)/tcp_server $(BUILD)/tcp_client

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/demo_driver: src/demo_driver.c $(CORE_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD)/tcp_server: src/tcp_server.c src/netproto.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD)/tcp_client: src/tcp_client.c src/netproto.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD)/test_device_model: tests/test_device_model.c src/device_model.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD)/test_ring_buffer: tests/test_ring_buffer.c src/ring_buffer.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD)/test_mmio: tests/test_mmio.c src/mmio.c src/device_model.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD)/test_netproto: tests/test_netproto.c src/netproto.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: $(BUILD)/test_device_model $(BUILD)/test_ring_buffer $(BUILD)/test_mmio $(BUILD)/test_netproto
	$(BUILD)/test_device_model
	$(BUILD)/test_ring_buffer
	$(BUILD)/test_mmio
	$(BUILD)/test_netproto

integration-test: all
	bash tests/test_tcp_integration.sh
	$(BUILD)/demo_driver

clean:
	rm -rf $(BUILD)
	rm -f /dev/shm/pcie_* 2>/dev/null || true
