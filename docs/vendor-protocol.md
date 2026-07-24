# Vendor USB Protocol

Single source of truth for the vendor-specific USB requests exposed by this
firmware. Both the firmware (`Usb/usb_vendor_cmd.c/h`, `Usb/usb_vendor_bulk.c/h`)
and the host tool (`tools/`) must match this document exactly. If the two
disagree, this document wins and the code is the bug.

## Device identity

| Field       | Value                          |
|-------------|---------------------------------|
| idVendor    | 0x0483 (1155)                   |
| idProduct   | 0x572B (22315)                  |
| Interfaces  | 0 = HID keyboard, 1 = CDC ACM control, 2 = CDC ACM data, 3 = Vendor bulk |

## EP0 vendor control requests

All requests use `bmRequestType = 0xC0` (control IN, device-to-host, vendor
type, recipient device). Parameters are carried in `wValue` / `wIndex`.
Every request returns a fixed-size response struct so the host always gets
an explicit success/failure signal - there is no request that returns zero
bytes.

Multi-byte fields are little-endian (native STM32 and x86/x64 byte order).
All response structs are packed (no padding).

| bRequest | Name                | wValue          | wIndex | Response struct        | Status |
|----------|---------------------|-----------------|--------|-------------------------|--------|
| 0x01     | GET_FIRMWARE_INFO   | -               | -      | `FirmwareInfo_t`        | implemented |
| 0x02     | SET_REPEAT_ENABLE   | 0=off, 1=on     | -      | `VendorResponse_t`      | implemented |
| 0x03     | SET_LED_MODE        | 0-3 (see enum)  | -      | `VendorResponse_t`      | implemented |
| 0x04     | START_RAM_DUMP      | -               | -      | `VendorDumpResponse_t`  | implemented - returns status=ERROR, acceptedLength=0 if a dump is already in progress |

### `VendorResponse_t` (2 bytes)

```c
typedef struct __attribute__((packed))
{
  uint8_t status;   /* 0 = VENDOR_STATUS_OK, 1 = VENDOR_STATUS_ERROR */
  uint8_t request;  /* echoed bRequest, so the host can match response to command */
} VendorResponse_t;
```

Used by `SET_REPEAT_ENABLE` and `SET_LED_MODE`.

### `VendorDumpResponse_t` (6 bytes)

```c
typedef struct __attribute__((packed))
{
  uint8_t  status;          /* 0 = OK, 1 = ERROR */
  uint8_t  request;         /* echoed bRequest (0x04) */
  uint32_t acceptedLength;  /* bytes the host should read from the vendor bulk IN endpoint. 0 when status = ERROR */
} VendorDumpResponse_t;
```

Used by `START_RAM_DUMP`.

### `FirmwareInfo_t` (17 bytes)

```c
typedef struct __attribute__((packed))
{
  uint32_t magic;               /* FW_INFO_MAGIC = 0x4C304753 ("SG0L" little-endian) */
  uint16_t versionMajor;
  uint16_t versionMinor;
  uint32_t featureFlags;        /* FW_FEATURE_* bitmask, see below */
  uint8_t  hidInterface;        /* interface number: 0 */
  uint8_t  cdcControlInterface; /* interface number: 1 */
  uint8_t  cdcDataInterface;    /* interface number: 2 */
  uint8_t  hidInEp;             /* 0x81 */
  uint8_t  cdcLogInEp;          /* 0x83 */
} FirmwareInfo_t;
```

Used by `GET_FIRMWARE_INFO`. The host validates the response by checking
`magic == FW_INFO_MAGIC` before trusting the rest of the struct.

Byte offsets (packed, little-endian):

| Offset | Field                | Size |
|--------|----------------------|------|
| 0      | magic                | 4    |
| 4      | versionMajor         | 2    |
| 6      | versionMinor         | 2    |
| 8      | featureFlags         | 4    |
| 12     | hidInterface         | 1    |
| 13     | cdcControlInterface  | 1    |
| 14     | cdcDataInterface     | 1    |
| 15     | hidInEp              | 1    |
| 16     | cdcLogInEp           | 1    |

Total: 17 bytes.

`featureFlags` bitmask:

| Bit | Flag                     |
|-----|--------------------------|
| 0   | FW_FEATURE_HID_KEYBOARD  |
| 1   | FW_FEATURE_CDC_LOG       |
| 2   | FW_FEATURE_VENDOR_BULK   |
| 3   | FW_FEATURE_REPEAT_CONTROL|

`VendorLedMode_t` enum (used for `SET_LED_MODE`'s `wValue`):

| Value | Mode            |
|-------|-----------------|
| 0     | LED_MODE_OFF    |
| 1     | LED_MODE_ON     |
| 2     | LED_MODE_BLINK_SLOW (toggles every 500 ms) |
| 3     | LED_MODE_BLINK_FAST (toggles every 125 ms) |

Any `wValue` outside 0-3 returns `VendorResponse_t.status = VENDOR_STATUS_ERROR`
and leaves the LED mode unchanged.

## Vendor bulk IN endpoint

RAM dump data streams over a dedicated bulk IN endpoint rather than EP0,
since control transfers are not suited to bulk data volumes.

| Field           | Value |
|------------------|---------------|
| Endpoint address | 0x84 (EP4 IN) |
| Type             | Bulk          |
| Max packet size  | 64 bytes      |
| Interface        | 3 (Vendor Specific, class 0xFF) |
| PMA offset       | 0x150 |
| Dump region      | Fixed: `0x20000000` (SRAM1 base), 144 KB (144\*1024 = 147456 bytes) |

Flow: host sends `START_RAM_DUMP`. If accepted (no dump already in
progress), `acceptedLength` is always `147456` and firmware streams exactly
that many bytes over EP 0x84 in 64-byte chunks, starting on the next main
loop iteration after the EP0 response. **No zero-length packet is sent** to
terminate the transfer, even though `147456 / 64 = 2304` is an exact
multiple of the max packet size - the host must read exactly
`acceptedLength` bytes and stop; it cannot rely on a short/zero packet as
an end marker. If `START_RAM_DUMP` is requested while a dump is already in
progress, the firmware rejects it (`status=ERROR, acceptedLength=0`)
without preempting or queuing the in-flight dump.

**A USB bus reset or suspend/resume always aborts an in-flight dump.**
The firmware does not attempt to resume a bulk transfer left mid-flight
across either event - the host must reissue `START_RAM_DUMP` and read the
full `acceptedLength` bytes again from the start. Host tooling should treat
a bulk read that stalls or errors mid-transfer as a sign the connection was
interrupted, and retry the whole dump rather than trying to resume a
partial read.

## Change log

- Vendor bulk endpoint and EP0 `START_RAM_DUMP` handler implemented
  (previously planned/stub). See `DEVLOG.md` (local) for milestone history.
