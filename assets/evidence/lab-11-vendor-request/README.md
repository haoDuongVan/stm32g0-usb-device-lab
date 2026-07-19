# lab-11-vendor-request — evidence

Proves: build succeeds standalone (no dependency on the vendor bulk endpoint added in `lab-12`).

## Files
- `build-ok.txt` — build console log.

## Note
Functional evidence (`GET_FIRMWARE_INFO`/`SET_REPEAT_ENABLE` output, `START_RAM_DUMP` stub error) is
deferred to `lab-12`: the host tool (`tools/vendor_test.py`) only exists from `lab-13` onward in this
repo's history, so testing this milestone in isolation would need borrowing it out-of-band — not
convincing as evidence on its own. Captured together with `lab-12`'s real dump instead.
