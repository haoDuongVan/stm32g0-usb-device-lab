# lab-15-usb-lifecycle — evidence

## Files
- `build-ok.txt` — build console log, `0 errors, 0 warnings`.

## Not yet tested on hardware

This milestone is committed with build evidence only - functional test evidence (bus reset,
reset mid-dump, suspend/resume) is deferred for now, not skipped for good:

- **Reset** and **suspend/resume** are straightforward to capture (unplug/replug, or Sleep
  and wake) whenever evidence work on this repo resumes.
- **Reset mid-dump** (the case that actually proves `VendorDump_Abort()` fires) needs care:
  the 144 KB dump completes in ~0.24 s on real hardware (see `lab-14` evidence), far too fast
  to unplug the cable by hand mid-transfer. A host-side script using `usb.core.Device.reset()`
  (a real USB port reset via the host controller, precisely timeable in code) was prototyped
  and works in principle, but was set aside for now along with the rest of the test pass.

See `DEVLOG.md` (`lab-15-usb-lifecycle` section) for the full design, the code changes, and
the test procedure to follow when this evidence is captured.
