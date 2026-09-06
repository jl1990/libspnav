#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT HUP INT TERM
${CC:-cc} -g -Wall ${TEST_CFLAGS:-} -DSPNAV_CONFIG_H_ -ffunction-sections -fdata-sections \
 -Isrc tests/test_lcd.c -Wl,--gc-sections -o "$build/test_lcd"
"$build/test_lcd"

${CC:-cc} -g -Wall ${TEST_CFLAGS:-} -DSPNAV_CONFIG_H_ -ffunction-sections -fdata-sections \
 -Isrc tests/test_connection.c -Wl,--gc-sections -o "$build/test_connection"
"$build/test_connection"
${CC:-cc} -g -Wall ${TEST_CFLAGS:-} -DSPNAV_CONFIG_H_ -ffunction-sections -fdata-sections \
 -Isrc tests/test_request_io.c -Wl,--gc-sections -o "$build/test_request_io"
"$build/test_request_io"
