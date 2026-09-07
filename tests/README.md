Run `sh tests/run.sh` to test the LCD API against a fake socket daemon. No
hardware or installed daemon is used. Set TEST_CFLAGS for sanitizer flags.

The matching spacenavd fork exposes Linux Enterprise screen controls:

- spnav_cfg_set_lcd / spnav_cfg_get_lcd: SPNAV_LCD_ENABLED and SPNAV_LCD_PROFILE.
- spnav_cfg_set_lcd_brightness / spnav_cfg_get_lcd_brightness: 0..100 percent.
- spnav_cfg_set_lcd_idle / spnav_cfg_get_lcd_idle: 0..86400 seconds; 0 means never.
- spnav_lcd_refresh: apply settings and redraw; returns actual update success/error.

Getters return -1 on error or unsupported daemon. Setters return 0 when the
configuration is accepted, -1 on error. Save with spnav_cfg_save to persist.
Manual off does not wake from input; automatic sleep wakes on Enterprise input.
The experimental request range 0x3f00..0x3f09 is shared with this daemon fork.

LED sleep uses spnav_cfg_set_led_idle / spnav_cfg_get_led_idle, with the same
0..86400 second range. It is independent of the LCD timer.

spnav_set_focus is for compositor/session helpers, not ordinary application
registration. It sends an acknowledged app-ID string of at most 255 UTF-8 bytes.
An empty ID means no focus; closing the connection releases provider ownership.
Tests cover string chunking, empty IDs, and input validation. GNOME adapter
activation and native Wayland profile switching await a login-session test.

Connection tests additionally check file/event-queue cleanup, descriptor zero,
closed-peer handling, interrupted reads, and deadlines for partial responses.

Profile snapshots are declared in `spnav_profiles.h`: read, edit the copied
snapshot, and apply it. Index zero is Default; application profiles follow.
`source_index` retains legacy settings during rename/duplication. Keep it from the
original snapshot (use zero for a new profile). After a successful apply, refresh
indices to their new positions or read a new snapshot. Returns: 0 success,
-1 invalid/unavailable, -2 revision conflict, -3 unsupported key.

The local wire layout contains only 32-bit integers and fixed-size character
arrays, uses native byte order like the existing protocol, and starts with a
version and revision. Do not store the binary struct as a persistent file format.
Transfers use BEGIN (0x3f10), READ/WRITE chunks of 24 bytes, APPLY, ACTIVE, FOCUS,
and CAPTURE (0x3f16). BEGIN returns the exact struct size. Writes must start at
offset zero and proceed in order; APPLY requires the complete snapshot. CAPTURE
suppresses mappings for ten seconds to identify physical buttons. Snapshot tests
also inject device events between protocol responses to ensure inputs do not
corrupt a transfer.
