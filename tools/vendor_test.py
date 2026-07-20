"""
vendor_test.py - EP0 vendor request smoke-test for stm32g0-usb-device-lab.

Exercises all vendor commands defined in docs/vendor-protocol.md:
  - GET_FIRMWARE_INFO  : read firmware info struct (magic, version, features, EP map)
  - SET_LED_MODE       : control the green LED (off / on / blink slow / blink fast)
  - SET_REPEAT_ENABLE  : enable or disable HID key-repeat events
  - START_RAM_DUMP     : trigger RAM dump stream via the vendor bulk IN endpoint

All commands use bmRequestType = 0xC0 (Vendor | Device | IN). Firmware always
returns a response struct so the tool has an explicit success/failure signal
without parsing the CDC log.

Setup:
    pip install -r requirements.txt

On Windows: install libusbK on Interface 3 (Vendor Data) using Zadig:
    Options -> List All Devices -> select Interface 3 -> install libusbK.
    HID and CDC keep their inbox drivers and remain fully functional.

Usage:
    python vendor_test.py
"""

import sys
import time

from vendor_usb import (
    VendorUsb,
    LED_OFF, LED_ON, LED_BLINK_SLOW, LED_BLINK_FAST,
)

LED_NAMES = {LED_OFF: "OFF", LED_ON: "ON",
             LED_BLINK_SLOW: "BLINK_SLOW", LED_BLINK_FAST: "BLINK_FAST"}


def cmd_get_firmware_info(usb: VendorUsb) -> None:
    """GET_FIRMWARE_INFO - print the decoded FirmwareInfo_t, or report failure."""
    info = usb.get_firmware_info()
    if info is None:
        print("  [ERR] no response, short response, or bad magic")
        return
    print(f"  [OK] firmware v{info['version']}")
    print(f"       features  : {info['features']}")
    print(f"       interfaces: HID={info['hid_if']} CDC_ctrl={info['cdc_ctrl_if']} "
          f"CDC_data={info['cdc_data_if']}")
    print(f"       endpoints : HID_IN={info['hid_ep']} CDC_IN={info['cdc_ep']}")


def cmd_set_led_mode(usb: VendorUsb, mode: int) -> None:
    """SET_LED_MODE - drive the green LED and print success/failure."""
    ok = usb.set_led_mode(mode)
    tag = "OK" if ok else "ERR"
    print(f"  [{tag}] led_mode -> {LED_NAMES.get(mode, mode)}")


def cmd_set_repeat_enable(usb: VendorUsb, enable: bool) -> None:
    """SET_REPEAT_ENABLE - toggle HID key-repeat forwarding and print the result."""
    ok = usb.set_repeat_enable(enable)
    tag = "OK" if ok else "ERR"
    print(f"  [{tag}] repeat_enable -> {enable}")


def cmd_start_ram_dump(usb: VendorUsb, output_file: str = "ram_dump.bin") -> None:
    """START_RAM_DUMP - stream the RAM dump and save it to a file.

    Prints [ERR] if a dump is already in progress (firmware rejects the
    request with VENDOR_STATUS_ERROR / acceptedLength=0 in that case).
    """
    def progress(received: int, total: int) -> None:
        print(f"\r  {received}/{total} bytes ({received * 100 // total}%)",
              end="", flush=True)

    result = usb.start_ram_dump(progress_cb=progress)
    if result is None:
        print("  [ERR] dump rejected (already in progress?)")
        return

    data, elapsed = result
    print()  # newline after progress
    throughput_kbs = (len(data) / 1024) / elapsed if elapsed > 0 else 0
    print(f"  {elapsed:.2f} s  ->  {throughput_kbs:.1f} KB/s")

    with open(output_file, "wb") as f:
        f.write(data)
    print(f"  saved {len(data)} bytes -> {output_file!r}")


def main() -> None:
    usb = VendorUsb()
    ok, product = usb.open()
    if not ok:
        print(f"Device not found ({usb_id_str()}).")
        print("Check that firmware is running and, once implemented, that")
        print("libusbK is installed on Interface 3 via Zadig.")
        sys.exit(1)
    print(f"Found: {product!r} ({usb_id_str()})")

    print("\n[1] GET_FIRMWARE_INFO")
    cmd_get_firmware_info(usb)

    print("\n[2] SET_LED_MODE: ON -> BLINK_SLOW -> BLINK_FAST -> OFF")
    cmd_set_led_mode(usb, LED_ON)
    time.sleep(1)
    cmd_set_led_mode(usb, LED_BLINK_SLOW)
    time.sleep(2)
    cmd_set_led_mode(usb, LED_BLINK_FAST)
    time.sleep(2)
    cmd_set_led_mode(usb, LED_OFF)

    print("\n[3] SET_REPEAT_ENABLE = False")
    cmd_set_repeat_enable(usb, False)
    print("     Hold any keypad key for 2 s - repeat should NOT fire.")
    time.sleep(2)

    print("\n[4] SET_REPEAT_ENABLE = True")
    cmd_set_repeat_enable(usb, True)
    print("     Hold any keypad key for 2 s - repeat should fire again.")
    time.sleep(2)

    print("\n[5] START_RAM_DUMP -> ram_dump.bin")
    cmd_start_ram_dump(usb)

    usb.close()
    print("\nDone.")


def usb_id_str() -> str:
    from vendor_usb import VID, PID
    return f"{VID:#06x}:{PID:#06x}"


if __name__ == "__main__":
    main()
