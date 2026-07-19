# lab-10-cdc-debug-channel — evidence

Proves: the CDC debug log channel (ring buffer + `CdcLog_Printf`) works, and the keyboard pipeline
actually logs through it. On connect, Tera Term receives `[CDC] log channel connected` shortly after
`SET_CONTROL_LINE_STATE` (DTR), confirming the delayed-greeting design actually lands after the terminal
is ready to read it. Every HID key event and the simultaneous-key error also log through the same channel.

## Files
- `build-ok.txt` — build console log.
- `composite-device-enumration-and-first-log.txt` — USBPcap capture: full enumeration, then
  `SET_CONTROL_LINE_STATE` (Value=3) when Tera Term opens the port, followed shortly after by the
  bulk IN transfer carrying `[CDC] log channel connected`.
- `usb-cdc-log-channel.png` — Tera Term screenshot: `[CDC] log channel connected`, then `[HID]
  keyLoc=.. event=.. usage=0x..` for each of the 16 keys pressed in turn plus one repeat, then `[ERR]
  simultaneous key detected - ErrorRollOver` when 2 keys were pressed together.
