# Evidence — organization

Each milestone (per the tag/name in `DEVLOG.md`) has its own subfolder containing:
- `README.md` — what this evidence proves, plus a short description of each file.
- Real files: `build-ok.txt`, `usbview-*.txt/png`, `wireshark-*.pcapng/png`, `teraterm-*.png`, links to demo videos, ...

## Evidence-type conventions (agreed)
- **USBView** → self-declared descriptor/interface/endpoint.
- **USBPcap/Wireshark** → actual traffic on the bus.
- **Tera Term** → CDC runtime log.
- **vendor_test.py** → host-side command/data path.
- **Video** → user-visible behavior (strongest evidence for the project page). Videos are not committed to this repo — each milestone's README links to the hosted (YouTube) copy instead.
- **WinMerge/diff** → illustrates a code change for the blog.
- **Device Manager** — supporting evidence only, never primary, since it can't distinguish between similar devices. Only use alongside USBView opened at the same time.

## One "hero" screenshot per milestone
Don't capture ten screenshots per step. Example: `lab-08` -> Wireshark HID report; `lab-09` -> USBView composite descriptor; `lab-10` -> Tera Term CDC log; `lab-12` -> vendor_test RAM dump; `lab-14` -> Tera Term lifecycle log.

## Tags
Every milestone has a git tag (see the table in `DEVLOG.md` section 1) — use `git checkout <tag>` to rebuild/re-capture evidence for any milestone.
