# lab-09-composite-skeleton — evidence

Proves: hand-written composite class replacing `usbd_hid.c` — step 1 (HID only), then step 2 (add CDC ACM).
Enumerates correctly, no USBView errors, CDC COM port appears.

## Files
- `build-ok-hid-only.txt` — build console log for step 1 (HID only, before adding CDC).
- `build-ok.txt` — build console log for step 2 (HID+CDC, final state of this milestone).
- `usbview-composite-descriptor.txt` — descriptor after step 2: `bDeviceClass=0xEF`, `bNumInterfaces=3`,
  `wTotalLength=100 (0x64)`, Interface 0 = HID (Keyboard protocol, 45-byte report descriptor),
  Interface 1-2 = CDC (via IAD).
- `usb-cdc-appear-in-tera-term.png` — Tera Term recognizing the new CDC COM port (not runtime log yet —
  that's `lab-10`).
- `diff-enable-sof-fix.png` — fix for `Sof_enable` left `DISABLE` in `USBD_LL_Init`.
- `diff-pma-btable-fix.png` — fix for PMA addresses overlapping the BTABLE region (`0x18/0x58/0x100` -> `0x040/0x080/0x0C0`).
- `diff-static-malloc-size-fix.png` — fix for `USBD_static_malloc` allocating the wrong size (`sizeof(HID handle)` -> `sizeof(Composite handle)`).
- `diff-device-class-iad-fix.png` — fix for `bDeviceClass/SubClass/Protocol` needing to be `0xEF/0x02/0x01`
  for IAD (bug reported directly by USBView once CDC was added).
- `diff-add-cdc-pma-buffer-fix.png` — added PMA config for EP2/EP3 (CDC) in step 2.

## Note
No USBView descriptor capture for step 1 alone: the first attempt showed a stale Windows-cached
descriptor (wrongly reporting Mouse protocol/74-byte descriptor, even though the source at this commit
already has the correct 8-byte keyboard descriptor — confirmed directly against `usbd_composite.c`).
Not used as evidence since it doesn't reflect the actual state; the build log is unaffected by USB
descriptor caching so it's kept.
