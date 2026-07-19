# lab-09-composite-skeleton — evidence

Step 1 of 2: hand-written composite class replacing `usbd_hid.c`, HID only (no CDC yet).
Proves the composite class mechanism (Init/DeInit/Setup/DataIn/SOF/GetCfgDesc) works correctly
before adding more complexity. Step 2 (CDC ACM) evidence is added when that step is committed.

## Files
- `build-ok-hid-only.txt` — build console log for this step.
- `diff-enable-sof-fix.png` — fix for `Sof_enable` left `DISABLE` in `USBD_LL_Init`.
- `diff-pma-btable-fix.png` — fix for PMA addresses overlapping the BTABLE region (`0x18/0x58/0x100` -> `0x040/0x080/0x0C0`).
- `diff-static-malloc-size-fix.png` — fix for `USBD_static_malloc` allocating the wrong size (`sizeof(HID handle)` -> `sizeof(Composite handle)`).

## Note
No USBView descriptor capture for this step alone: the first attempt showed a stale Windows-cached
descriptor (wrongly reporting Mouse protocol/74-byte descriptor, even though the source at this commit
already has the correct 8-byte keyboard descriptor — confirmed directly against `usbd_composite.c`).
Not used as evidence since it doesn't reflect the actual state; the build log is unaffected by USB
descriptor caching so it's kept.
