# lab-08-hid-keyboard-baseline — evidence

Proves: the real USB keyboard works end-to-end — correct Boot Keyboard descriptor (no longer CubeMX's
default mouse descriptor), correct 8-byte report on key press, null report after release, key repeat,
macro sequences, and ErrorRollOver on simultaneous press.

## Files
- `build-ok.txt` — build console log.
- `usbview-hid-descriptor.txt` / `usbview-hid-descriptor.png` — correct Boot Keyboard descriptor:
  `bInterfaceClass=03 HID`, `bInterfaceSubClass=01 Boot`, `bInterfaceProtocol=01 Keyboard`,
  `wMaxPacketSize=8`, `wDescriptorLength=0x2D` (45 bytes).
- `wireshark-enumeration.png` — enumeration capture.
- `wireshark-report-press-1.png` — report when pressing key "1": `00 00 1E 00 00 00 00 00`.
- `wireshark-null-report.png` — null report after release.
- `wireshark-rollover-report.png` — ErrorRollOver when pressing >=2 keys simultaneously.
- Demo video: https://youtu.be/VsBNv4Ml51Q (source file kept local-only, not committed). Real hardware,
  no audio, on-screen captions. Three synced views (Wireshark capture, Notepad, camera on the physical
  keypad) plus a translucent key-map overlay. Shows: USB enumeration on plug-in; all 16 keys pressed in
  turn (digits, letters, Enter/Space/Backspace/Tab) with the report bytes and resulting Notepad character
  side by side; holding a key to show ~200 ms repeat; the 4 macro keys (Ctrl+C/V/S, Alt+Tab) firing their
  multi-step HID sequences; and pressing 2 keys together to trigger ErrorRollOver.
- `hid-keyboard-demo-capture.pcapng` / `hid-keyboard-demo-capture.txt` — USBPcap capture recorded
  alongside the video, cross-referenced against the reports seen on screen.

## Wireshark filter note
`usb.transfer_type == 0x01` (Interrupt), or filter by the device's specific Interrupt IN endpoint.
