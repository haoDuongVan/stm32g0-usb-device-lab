# lab-06-keyboard-pipeline — evidence

Proves: debounce, press/release events, key repeat, and the simultaneous-key error policy all work correctly.
This is the first milestone where `KeyDetect_Run()` actually runs in the main loop, so the
raw scan -> debounce -> key detect -> event queue pipeline is meaningful to record for the first time
(no key-press video in `lab-05` since the pipeline wasn't wired in yet).

## Files
- `build-ok.txt` — build console log, `0 errors, 0 warnings`.
- Demo video: https://youtu.be/SdR-xoFYoiA (source file kept local-only, not committed). No audio,
  on-screen text captions instead. Recorded with breakpoints halting on each cycle/event for the local
  variables (`rawState`/`pressedCount`), and Live Expressions for the static/global state while free-running.

  What the video shows, in order:
  - State is fully zeroed right after init (`sScanBuffer`, `sKeyStatus`, `sRepeatTick`).
  - Free-running, `sScanTickTotal` vs `uwTick` confirms TIM6 drives a scan roughly every 5 ms.
  - Each physical key maps to one bit in `rawState`.
  - The debounce history in `sScanBuffer` shifts back one slot every scan cycle.
  - After 2 consecutive cycles read a key as pressed, `sKeyStatus` sets the bit and a `KEY_EVENT_ON` is pushed.
  - Holding the key, `sRepeatTick` climbs to the 200 ms threshold and fires `KEY_EVENT_REPEAT` repeatedly.
  - After 2 consecutive cycles read the key as released, `sKeyStatus` clears the bit and a `KEY_EVENT_OFF` is pushed.
  - Pressing 2 keys together trips the simultaneous-key error: `KEY_EVENT_ERROR` fires once, `sSimultaneousErrorActive` latches true.
  - Releasing only one key still returns early (still latched, no event). Releasing all keys resets state silently.
