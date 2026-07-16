# lab-05-keypad-scan-layer — evidence

Proves: `Hardware/keypad.c/h` (raw matrix scan, direct BSRR/IDR) and `Hardware/scan_scheduler.c/h`
were added correctly to the Hardware layer. Build OK.

**Scope note**: this milestone does **not** wire `KeyDetect_Run()` into the main loop yet
(`HID_Keyboard_App()` only has `// TODO: Read keypad matrix...`), so there is **no** real
key-press video/demo here. Verifying real input (raw scan -> debounce -> key detect -> event queue)
moves to `lab-06-keyboard-pipeline`, once the pipeline is actually meaningful to record.

## Files
- `build-ok.txt` — build console log.
- `keypad-readraw-source.png` — source screenshot of `Keypad_ReadRaw` in `Hardware/keypad.c`.
- `scan-scheduler-source.png` — source screenshot of `Hardware/scan_scheduler.c`.
- `source-tree-hardware-layer.png` — source tree screenshot of the Hardware layer.
