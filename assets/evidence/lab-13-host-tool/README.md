# lab-13-host-tool — evidence

Proves: `tools/vendor_test.py` drives the full vendor protocol end-to-end against real hardware —
`GET_FIRMWARE_INFO`, `SET_LED_MODE`, `SET_REPEAT_ENABLE`, and `START_RAM_DUMP` (real 144 KB dump,
not the lab-11 stub).

## Files
- `vendor-test-output.txt` — full CLI run: firmware info, LED mode cycling, repeat enable toggling,
  then the RAM dump (progress log trimmed, full byte count / speed / saved file kept).
- `host-tool-run.png` — composite screenshot: source tree with the saved `ram_dump.bin`, a hex viewer
  on its contents, the terminal running `vendor_test.py`, and a side-by-side Tera Term window showing
  the matching `[VREQ]`/`[BULK]` CDC log lines for each command.
