# Old Project 05/06 File Classification

Source read from:
- `stm32g0b1re-firmware-debugging-lab/firmware/05_usb_hid_keyboard/` (HID keyboard baseline)
- `stm32g0b1re-firmware-debugging-lab/firmware/06_usb_composite_cdc_hid_vendor/` (composite CDC + HID + vendor + bulk)

Project 06 is a superset of Project 05 (same keypad/HID files, plus `cdc_log.*`, `vendor_cmd.*`, `usbd_composite.*`, and `tools/`). Classification below is based on Project 06; Project 05 maps to the same rows minus the CDC/vendor/bulk/composite ones.

This is a read-only classification. No files have been moved or modified.

## Generated / ST middleware

| Old path | Notes |
|---|---|
| `Core/Src/main.c`, `Core/Inc/main.h` | CubeMX-generated init + `USER CODE` sections calling `HID_Keyboard_Init`/`HID_Keyboard_App` |
| `Core/Src/stm32g0xx_*`, `Core/Startup/*` | Standard CubeMX/CMSIS startup |
| `Drivers/CMSIS/`, `Drivers/STM32G0xx_HAL_Driver/` | ST HAL/CMSIS, unchanged |
| `Middlewares/ST/STM32_USB_Device_Library/Core/*`, `Class/HID/*` | ST USB Device Library core + HID class |
| `USB_Device/App/usb_device.c/h`, `usbd_desc.c/h` | CubeMX-generated USB device init + descriptors |
| `USB_Device/Target/usbd_conf.c/h` | CubeMX-generated USBD low-level config |

## App layer → `App/app_main.c/h`

| Old path | Role | Blog article |
|---|---|---|
| `Core/Src/hid_keyboard_app.c` (functions `HID_Keyboard_Init`, `HID_Keyboard_App`, `HAL_TIM_PeriodElapsedCallback`) | App orchestration: init order, main-loop dispatch, TIM6 ISR → scan scheduler | 1. Intro |

**Mixed responsibility**: `hid_keyboard_app.c` also contains `UsbHidTransport_*` (see USB HID layer below). This file must be split, not moved as-is — matches hard rule 9 (mixed responsibility → identify, then split in a later milestone).

## Hardware layer → `Hardware/`

| Old path | New path | Role | Blog article |
|---|---|---|---|
| `Core/Src/matrix_scan.c/h` | `Hardware/keypad.c/h` (or `matrix_scan.c/h`) | Raw 4x4 matrix scan, row/col bit mapping | 4. Key Input Pipeline |
| `Core/Src/scan_scheduler.c/h` | `Hardware/scan_scheduler.c/h` | TIM6-driven 5ms tick counter, consumed by main loop | 4. Key Input Pipeline |

Note: the sample files already present in this new repo (`App/app_main.c`, `Hardware/keypad.c`, `Hardware/scan_scheduler.c`) are an earlier hand-adaptation of these exact old files — naming already matches this plan.

## Keyboard layer → `Keyboard/`

| Old path | New path | Role | Blog article |
|---|---|---|---|
| `Core/Src/key_detect.c/h` | `Keyboard/key_detect.c/h` | Debounce raw scan, emit press/release events (incl. `KEY_LOC_ERROR_ROLLOVER`) | 4. Key Input Pipeline |
| `Core/Src/key_event_queue.c/h` | `Keyboard/key_event_queue.c/h` | Fixed-size (32) key event queue between detect and convert | 4. Key Input Pipeline |
| `Core/Src/key_table.c/h` | `Keyboard/key_table.c/h` | keyLoc → HID usage/modifier lookup table | 4/5. Key Input, HID Baseline |
| `Core/Src/hid_keyboard_convert.c/h` | `Keyboard/hid_keyboard_convert.c/h` | Drain event queue, build HID report, call transport send | 5. HID Keyboard Baseline |
| `Core/Src/hid_keyboard_report.c/h` | `Keyboard/hid_keyboard_report.c/h` | 8-byte HID report struct/builder, modifier + keycode[6] handling | 5. HID Keyboard Baseline |

## USB HID layer → `Usb/usb_hid_keyboard.c/h`

| Old path | Role | Blog article |
|---|---|---|
| `Core/Src/hid_keyboard_app.c` (functions `UsbHidTransport_Init`, `UsbHidTransport_IsIdle`, `UsbHidTransport_SendReport`, `UsbHidTransport_TxCpltCallback`) | HID IN endpoint BUSY/IDLE state machine, atomic state via `__disable_irq`/`__enable_irq`, wraps `USBD_COMPOSITE_HID_SendReport` | 5. HID Keyboard Baseline |

**Extraction note**: these functions live in the same file as the App-layer functions above and must be pulled out into their own translation unit at the HID transport milestone (lab-08), not before.

## CDC layer → `Usb/usb_cdc_log.c/h`

| Old path | Role | Blog article |
|---|---|---|
| `Core/Src/cdc_log.c/h` | Ring buffer (512B) logger; `CdcLog_Printf`/`CdcLog_Write` fill it, `CdcLog_Run` drains to CDC IN endpoint from the main loop only (not ISR-safe) | 6. CDC Debug Channel |

## Vendor/Bulk layer → split into `Usb/usb_vendor_cmd.c/h` + `Usb/usb_vendor_bulk.c/h`

`vendor_cmd.c/h` mixes two distinct responsibilities and must be split per the rebuild plan:

| Old symbol | New path | Role | Blog article |
|---|---|---|---|
| `VendorCmd_Init`, `VendorCmd_GetRepeatEnable`, `VendorCmd_GetLedMode`, `VendorCmd_UpdateLed`, `VendorCmd_FlushPendingLog`, `VendorCmd_HandleSetup` (GET_FIRMWARE_INFO, SET_REPEAT_ENABLE, SET_LED_MODE) | `Usb/usb_vendor_cmd.c/h` | EP0 control-IN vendor requests, deferred CDC logging via pending-event flag (not ISR-safe to log directly) | 7. Vendor Request + Bulk |
| `VendorCmd_HandleSetup` (START_RAM_DUMP branch), `VendorDump_SendNextChunk`, `VendorDump_OnTxCplt`, `VendorDump_Run`, RAM dump state (`gDumpActive/Addr/Total/Offset/Done`) | `Usb/usb_vendor_bulk.c/h` | Bulk IN RAM dump state machine, 64-byte chunks, ISR-driven `OnTxCplt` + main-loop `Run` | 7. Vendor Request + Bulk |

Both halves share `FirmwareInfo_t`/`VendorResponse_t`/`VendorDumpResponse_t` and constants (`RAM_DUMP_BASE`, `COMP_VENDOR_*`) — the split needs a shared header or a small amount of duplication; decide at milestone 11/12 time, not now.

## Composite descriptor → `Usb/usbd_composite.c/h`

| Old path | New path | Notes |
|---|---|---|
| `USB_Device/App/usbd_composite.c/h` | `Usb/usbd_composite.c/h` | Custom composite class driver (HID+CDC+Vendor, IAD, endpoint map, setup dispatch). |

**Decision override**: the original rebuild plan defaulted to keeping this file under `USB_Device/App/` (it behaves like a class driver integrated with ST's USB Device middleware), but explicitly allowed moving it on request. Project owner has requested that **all self-written code live under `App/Hardware/Keyboard/Usb`**, with generated files (`USB_Device/App/usb_device.c`) only `#include`-ing headers from those layers when they need to call into user code (e.g. `USBD_RegisterClass(&hUsbDeviceFS, &USBD_Composite)`). Since `Usb/` is already on the include path (lab-03), no further build config change is needed when this file is actually created at milestone 10.

## Host tools → `tools/`

| Old path | Role | Blog article |
|---|---|---|
| `tools/vendor_test.py` | CLI vendor request tester (GET_FIRMWARE_INFO etc.) | 7. Vendor Request + Bulk |
| `tools/composite_debug_tool/vendor_usb.py` | pyusb wrapper class for the vendor commands | 7. Vendor Request + Bulk |
| `tools/composite_debug_tool/` (`main.py`, `gui.py`, `cdc_logger.py`, `hex_view.py`) | GUI composite debug tool: CDC log viewer + vendor command + RAM dump hex view | 8. Composite Debug Tool |
| `tools/ram_dump.bin` | Sample captured dump — evidence artifact, not tool code | 8. Composite Debug Tool |

Note: `__pycache__/` under `composite_debug_tool/` is build cache, not source — will not be copied.

**Update after completion (lab-13-host-tool)**: only the CLI part was actually built — `tools/vendor_usb.py` (renamed from the old path, no longer under the `composite_debug_tool/` subfolder) and `tools/vendor_test.py`, both sitting directly under `tools/`. The GUI (`main.py`/`gui.py`/`cdc_logger.py`/`hex_view.py`) was **not built** in this rebuild — out of scope for the `lab-13` milestone, possibly future work if needed.

## Docs/evidence

Old Project 06 has a `README.md` per firmware folder and a `Debug/` build directory (build artifacts, already gitignored pattern in this new repo). No dedicated `docs/`/`assets/` evidence folder existed in the old project — evidence conventions in this new repo (`assets/logs`, `assets/screenshots`, etc.) are new to this rebuild.

## Summary table (old → new)

| Old path | New path |
|---|---|
| `Core/Src/matrix_scan.c/h` | `Hardware/keypad.c/h` |
| `Core/Src/scan_scheduler.c/h` | `Hardware/scan_scheduler.c/h` |
| `Core/Src/key_detect.c/h` | `Keyboard/key_detect.c/h` |
| `Core/Src/key_event_queue.c/h` | `Keyboard/key_event_queue.c/h` |
| `Core/Src/key_table.c/h` | `Keyboard/key_table.c/h` |
| `Core/Src/hid_keyboard_convert.c/h` | `Keyboard/hid_keyboard_convert.c/h` |
| `Core/Src/hid_keyboard_report.c/h` | `Keyboard/hid_keyboard_report.c/h` |
| `Core/Src/hid_keyboard_app.c/h` (App part) | `App/app_main.c/h` |
| `Core/Src/hid_keyboard_app.c` (`UsbHidTransport_*`) | `Usb/usb_hid_keyboard.c/h` |
| `Core/Src/cdc_log.c/h` | `Usb/usb_cdc_log.c/h` |
| `Core/Src/vendor_cmd.c/h` (EP0 commands) | `Usb/usb_vendor_cmd.c/h` |
| `Core/Src/vendor_cmd.c/h` (RAM dump) | `Usb/usb_vendor_bulk.c/h` |
| `USB_Device/App/usbd_composite.c/h` | `Usb/usbd_composite.c/h` |
| `tools/vendor_test.py`, `tools/composite_debug_tool/` | kept under `tools/` |

## Proposed next milestone

`lab-05-keypad-scan-layer` — move/adapt `matrix_scan.c/h` and `scan_scheduler.c/h` into `Hardware/`. This is the safest starting point: no USB dependency, no debounce/event logic, and the equivalent files already exist in this repo's `Hardware/` folder in an earlier hand-adapted form, so the diff will be small and easy to verify against real hardware (raw matrix state on key press) before moving anything USB-related.

**Update after completing the full rebuild**: `lab-05` went exactly as proposed. From there on, the actual plan diverged from this document's original proposal in a few places:
- "7. Vendor Request + Bulk" was split into **2 separate git milestones**: `lab-11-vendor-request` (EP0 command framework, `START_RAM_DUMP` stub) then `lab-12-vendor-bulk` (real Interface 3/EP4 + wiring EP0 into the bulk path) — not merged into one milestone as originally proposed, so each commit can build independently.
- The host tool (`tools/vendor_usb.py`/`vendor_test.py`) became its own milestone `lab-13-host-tool`, placed **after** both `lab-11`/`lab-12` in the public git history (even though the tool was actually written earlier during development) — prioritizing readable ordering over the true chronological order.
- Added an entirely new milestone `lab-14-usb-lifecycle` (reset/suspend/resume hardening) — **not in the original plan** — which came up after discovering Project 06 had the same gap.
- The originally planned "Milestone 14" (final docs/evidence, no new code) — not done yet, still ahead.

See `DEVLOG.md` (local) for the full, commit-accurate history.
