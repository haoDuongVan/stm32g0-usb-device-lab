# lab-14-composite-debug-tool-gui — evidence

Proves: the ported GUI (`tools/composite_debug_tool/`) drives the full vendor protocol end to
end through the shared `tools/vendor_usb.py` layer — CDC log, all 4 EP0 vendor commands, and
the vendor bulk RAM dump — from a single window.

## Files
- `host-tool-result.png` — GUI screenshot after a full run: CDC log panel connected on COM7,
  device connected as `STM32 USB HID 4x4 Macro Keypad`, Key Repeat state `Enabled`, RAM Dump
  at `147456 bytes | 0.24 s | 598.6 KB/s`, hex preview in the Response/Dump Output panel.
- `cdc-demo-log.txt` — raw CDC log captured during the run: `GET_FIRMWARE_INFO`, HID key
  events with Repeat first disabled then enabled (single `[HID]` line per key vs. repeated
  `event=2` lines), all 4 `SET_LED_MODE` values, and the `[BULK]` start/complete pair for the
  RAM dump.
- `vendor-commands-log.txt` — GUI's own Response/Dump Output panel text: `GET_FIRMWARE_INFO`
  decode, both `SET_REPEAT_ENABLE` states, all 4 LED modes, and the RAM dump result with a
  512-byte hex preview — matches `cdc-demo-log.txt` from the independent CDC side.
- `usb-device-lab-14.mp4` — full demo recording (source file kept local-only, not committed —
  see note below).

## Video

Demo video: https://youtu.be/AYjgSDcFNJ8 (source file kept local-only, not committed).
