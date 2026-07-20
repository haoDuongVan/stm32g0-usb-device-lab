# lab-12-vendor-bulk — evidence

Proves: Interface 3 (Vendor Specific) + EP4 IN bulk enumerate correctly, added to the composite descriptor
alongside HID/CDC without breaking either.

## Files
- `build-ok.txt` — build console log.
- `usbview-descriptor.txt` — USBView descriptor: `bNumInterfaces=4`, new `EP4 IN 0x84` (Bulk, 64 bytes),
  Interface 0-2 (HID/CDC) unchanged from `lab-09`.
- `wireshark-composite-device-enumration.png` / `.txt` — USBPcap enumeration capture, confirms
  `INTERFACE DESCRIPTOR (3.0): class Vendor Specific` in the configuration descriptor.
