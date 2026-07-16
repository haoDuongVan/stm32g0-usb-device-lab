# lab-02-ioc-gpio-usb-config — evidence

Proves: keypad GPIO, TIM6 (5ms scan tick), and USB clock/pins/middleware are configured correctly at the CubeMX level.
build OK.
USB enumerates (though the report descriptor is still CubeMX's default mouse at this point).

## Files
- `build-ok.txt` — build console log.
- `usbview-default-hid-mouse.txt` / `usbview-default-hid-mouse.png` — USBView confirms enumeration,
  `bInterfaceClass=0x03 (HID)`, `bInterfaceProtocol=0x02` (Boot Mouse — expected, fixed in lab-08).
  The `.png` highlights the `bInterfaceProtocol: 0x02 -> Mouse` line.
- `cubemx-pinout-keypad-tim6-usb.png` — CubeMX Pinout & Configuration: `KEYPAD_ROW0-3`/`KEYPAD_COL0-3` (PB0-PB7),
  `USB_DP`/`USB_DM` (PA11/PA12), and the NVIC Interrupt Table showing both `USB, UCPD1 and UCPD2 global interrupts` 
  and `TIM6, DAC and LPTIM1 global Interrupts` enabled.
