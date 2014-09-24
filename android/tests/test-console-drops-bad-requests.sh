#!/bin/sh

# This test is designed to check that invalid requests on the console
# port get the connection dropped with a proper KO.

die () {
  echo "ERROR: $@"
  exit 1
}

# $1: First string
# $2: Second string
# $3: Error message (optional).
expect_streq () {
  local FIRST SECOND
  FIRST=$1
  SECOND=$2
  if [ "$FIRST" != "$SECOND" ]; then
    if [ "$3" ]; then
        printf "%s\n" "$3"
    fi
    echo "ERROR: String mismatch"
    echo "Expected: [$FIRST]"
    echo "Actual:   [$SECOND]"
    exit 1
  fi
}

case "$ANDROID_SERIAL" in
  emulator-*)
    EMULATOR_PORT=${ANDROID_SERIAL##emulator-}
    ;;
  *)
    EMULATOR_PORT=5554
    ;;
esac

CONSOLE_1=$(echo "quit" | nc localhost $EMULATOR_PORT)
expect_streq \
    "$(printf "Android Console: type 'help' for a list of commands\r\nOK\r\n")" \
    "$CONSOLE_1" \
    "No emulator console on port $EMULATOR_PORT"

CONSOLE_2=$(echo "GET /index.html HTTP/1.0\r\n" | nc localhost $EMULATOR_PORT | tail -1)
expect_streq \
    "$(printf "KO: Forbidden HTTP request. Aborting\r\n")" \
    "$CONSOLE_2" \
    "HTTP request was not dropped as expected."

CONSOLE_3=$(printf "Hello \x80\x80 World\r\n" | nc localhost $EMULATOR_PORT | tail -1)
expect_streq \
    "$(printf "KO: Forbidden binary request. Aborting\r\n")" \
    "$CONSOLE_3" \
    "Binary request was not dropped as expected."

echo "OK"
exit 0
