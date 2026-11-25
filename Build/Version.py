#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Print the release version of the checked-out Git commit."""

from datetime import datetime, timezone
from subprocess import check_output


def main() -> None:
    timestamp = int(check_output(["git", "show", "-s", "--format=%ct", "HEAD"]))
    commit = check_output(["git", "rev-parse", "--short=7", "HEAD"], text=True).strip()
    utc = datetime.fromtimestamp(timestamp, tz=timezone.utc)
    print(f"{utc:%Y%m%d}.{utc.hour * 100 + utc.minute}.{utc.second}+{commit}")


if __name__ == "__main__":
    main()
