"""
main.py - Entry point for the stm32g0-usb-device-lab composite debug GUI.

Usage:
    cd tools/composite_debug_tool
    pip install -r ../requirements.txt
    python main.py

Requirements:
    - libusbK installed on Interface 3 (Vendor Data) via Zadig
    - HID and CDC keep their inbox drivers
"""

import sys
from pathlib import Path

# vendor_usb.py is the shared protocol layer one level up (tools/vendor_usb.py),
# also used by tools/vendor_test.py - not duplicated here.
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from gui import App


def main():
    app = App()
    app.mainloop()


if __name__ == "__main__":
    main()
