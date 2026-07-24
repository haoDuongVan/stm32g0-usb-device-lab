# stm32g0-usb-device-lab

A from-scratch, milestone-by-milestone rebuild of a USB composite device (HID keyboard + CDC
debug log + vendor-specific bulk RAM dump) on an STM32G0B1RE-Nucleo board. Every milestone is
one clean commit and one git tag, with real hardware evidence (build logs, USBView/Wireshark
captures, terminal screenshots, host-tool output, demo videos) in `assets/evidence/`.

This is a split-out, cleaned-up rebuild of the USB composite project from
[stm32g0b1re-firmware-debugging-lab](https://github.com/haoDuongVan/stm32g0b1re-firmware-debugging-lab)
(`firmware/06_usb_composite_cdc_hid_vendor`), done as its own repo so the milestone history
and evidence stay readable on their own instead of buried in a larger multi-project repo. The
process is stricter than the original: read the reference code, understand *why* it works
before reusing it, and prove every claim with evidence instead of "it should work."

## Hardware

- Board: NUCLEO-G0B1RE (STM32G0B1RE, Cortex-M0+)
- 4x4 matrix keypad (no diodes - see the simultaneous-key error policy in `lab-06`)
- USB Type-A breakout board (target device's own USB connection, separate from the
  ST-Link USB port used for flashing/debugging)
- USB full-speed device: `idVendor=0x0483`, `idProduct=0x572B`

## What it does

- Enumerates as a 4-interface composite USB device: HID keyboard, CDC ACM (debug log), and a
  vendor-specific bulk interface (RAM dump).
- Debounced 4x4 keypad scanning with key repeat and a deliberate simultaneous-key error policy.
- A `printf`-style debug log streamed over the CDC interface - observable from any terminal,
  no debugger needed.
- An EP0 vendor command set (`GET_FIRMWARE_INFO`, `SET_REPEAT_ENABLE`, `SET_LED_MODE`,
  `START_RAM_DUMP`) plus a dedicated bulk endpoint that streams a full 144 KB SRAM dump.
- USB bus reset/suspend/resume handling that aborts any in-flight RAM dump instead of leaving
  the state machine stuck.
- Two host-side Python tools that speak the same protocol layer: a CLI (`tools/vendor_test.py`)
  and a tkinter GUI (`tools/composite_debug_tool/`).

## Milestones

| Tag | What it adds | Evidence |
|-----|---------------|----------|
| [`lab-01-cubemx-baseline`](assets/evidence/lab-01-cubemx-baseline) | CubeMX default project, board bring-up | build log |
| [`lab-02-ioc-gpio-usb-config`](assets/evidence/lab-02-ioc-gpio-usb-config) | Keypad GPIO, TIM6 scan tick, USB clock/pins | build log, USBView |
| [`lab-03-project-layer-structure`](assets/evidence/lab-03-project-layer-structure) | App/Hardware/Keyboard/Usb folder layout | build log |
| [`lab-04-old-code-classification`](assets/evidence/lab-04-old-code-classification) | Classify the reference project's files before reusing any of it | docs only |
| [`lab-05-keypad-scan-layer`](assets/evidence/lab-05-keypad-scan-layer) | Raw 4x4 matrix scan + 5 ms tick | build log |
| [`lab-06-keyboard-pipeline`](assets/evidence/lab-06-keyboard-pipeline) | Debounce, key events, repeat, simultaneous-error policy | debugger walkthrough [video](https://youtu.be/SdR-xoFYoiA) |
| [`lab-07-hid-report-layer`](assets/evidence/lab-07-hid-report-layer) | Key event -> 8-byte HID report, no transport yet | debugger walkthrough [video](https://youtu.be/aOZO3ETZRBc) |
| [`lab-08-hid-keyboard-baseline`](assets/evidence/lab-08-hid-keyboard-baseline) | Real USB keyboard end to end, macro sequences | Wireshark + hardware demo [video](https://youtu.be/VsBNv4Ml51Q) |
| [`lab-09-composite-skeleton`](assets/evidence/lab-09-composite-skeleton) | Hand-written composite class (HID + CDC), replacing `usbd_hid.c` | USBView |
| [`lab-10-cdc-debug-channel`](assets/evidence/lab-10-cdc-debug-channel) | CDC debug log channel (ring buffer + printf-style) | Tera Term |
| [`lab-11-vendor-request`](assets/evidence/lab-11-vendor-request) | EP0 vendor command framework | build log |
| [`lab-12-vendor-bulk`](assets/evidence/lab-12-vendor-bulk) | Vendor bulk interface + real RAM dump engine | USBView, Wireshark |
| [`lab-13-host-tool`](assets/evidence/lab-13-host-tool) | `vendor-protocol.md` + CLI host tool | CLI run output |
| [`lab-14-composite-debug-tool-gui`](assets/evidence/lab-14-composite-debug-tool-gui) | tkinter GUI: CDC log + all vendor commands + RAM dump in one window | screenshot, logs, demo [video](https://youtu.be/AYjgSDcFNJ8) |
| [`lab-15-usb-lifecycle`](assets/evidence/lab-15-usb-lifecycle) | Reset/suspend/resume never leaves the dump state machine stuck | build log |

Check out any tag to see the project exactly as it stood at that milestone:
`git checkout lab-08-hid-keyboard-baseline`.

## Repository structure

```
App/            Main loop, init
Hardware/       Keypad scan, scan scheduler
Keyboard/       Debounce, key events, HID report conversion
Usb/            Composite class, CDC log, vendor commands, vendor bulk, lifecycle
USB_Device/     CubeMX-generated USB device stack (hand-patched where noted)
docs/           Protocol reference (vendor-protocol.md)
tools/          Host-side Python: CLI test tool + GUI
assets/evidence/ Per-milestone evidence (build logs, captures, screenshots, video links)
```

## Host tools

```bash
cd tools
pip install -r requirements.txt

# CLI - runs all 4 vendor commands once
python vendor_test.py

# GUI - CDC log + vendor commands + RAM dump in one window
python composite_debug_tool/main.py
```

Windows requires the vendor interface (Interface 3) to be bound to `libusbK` via
[Zadig](https://zadig.akeo.ie/) - HID and CDC keep their inbox drivers, only the vendor
interface needs this.

Protocol details: [`docs/vendor-protocol.md`](docs/vendor-protocol.md).

## Building

CubeIDE project (`.cproject`/`.project`). Import as an existing project and build the
`Debug` or `Release` configuration.

## License

[MIT](LICENSE)
