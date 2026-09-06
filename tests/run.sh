#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT HUP INT TERM
${CC:-cc} -g -Wall ${TEST_CFLAGS:-} -DSPNAV_CONFIG_H_ -ffunction-sections -fdata-sections \
 -Isrc tests/test_lcd.c -Wl,--gc-sections -o "$build/test_lcd"
"$build/test_lcd"
