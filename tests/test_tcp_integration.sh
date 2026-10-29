#!/usr/bin/env bash
# Starts the real TCP server, runs the real TCP client against it over a
# real loopback socket, and checks the echoed message matches -- an
# integration-level proof that bind/listen/accept/connect and the framing
# protocol work together, on top of the unit-level socketpair tests in
# test_netproto.c.
set -euo pipefail

BUILD_DIR="$(dirname "$0")/../build"
PORT=9191
MESSAGE="integration-test-message"

"$BUILD_DIR/tcp_server" "$PORT" &
SERVER_PID=$!
trap 'kill $SERVER_PID 2>/dev/null || true' EXIT

sleep 0.3

OUTPUT="$("$BUILD_DIR/tcp_client" "$PORT" "$MESSAGE")"
wait $SERVER_PID

EXPECTED="echo: $MESSAGE"
if [ "$OUTPUT" != "$EXPECTED" ]; then
    echo "FAIL: expected '$EXPECTED', got '$OUTPUT'"
    exit 1
fi

echo "PASS: tcp integration round trip ($OUTPUT)"
