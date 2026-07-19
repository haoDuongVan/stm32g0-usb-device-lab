# lab-07-hid-report-layer — evidence

Proves: key event -> 8-byte HID report is built correctly, null report after release, ErrorRollOver —
all without going through real USB yet (transport is `lab-08`).

## Files
- `build-ok.txt` — build console log, `0 errors, 0 warnings`.
- Demo video: https://youtu.be/aOZO3ETZRBc (source file kept local-only, not committed). No audio,
  on-screen text captions instead. Recorded with breakpoints halting at each report-building step,
  since `sReport` is only "live" for a single main-loop iteration before the null report clears it.

  What the video shows, in order:
  - Report is all zero right after `HidKeyboardConvert_Init()`.
  - `KEY_EVENT_ON` -> `HidKeyboardConvert_BuildKeyReport` sets `bytes[2] = 30` (decimal) = `0x1E` = `HID_USAGE_1`.
  - The next call clears the report back to null (tap-style, not held) via `sNeedNullReport`.
  - `KEY_EVENT_OFF` is popped from the queue but changes nothing — the release was already scheduled at ON time.
  - Holding the key: `KEY_EVENT_REPEAT` rebuilds the same report every ~200 ms.
  - Pressing 2 keys together: `KEY_EVENT_ERROR` fills `bytes[2..7]` with `0x01` (HID Boot Keyboard ErrorRollOver).
