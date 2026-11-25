#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Copy shared evidence from a Godot Apartment save into a new native save."""

import argparse
import json
from pathlib import Path


NATIVE_SOURCE = {0: 1, 1: 3, 2: 4, 3: 2, 5: 6}
REQUIRED_SOURCES = {0: {0, 1}, 1: {2, 3}, 2: {4, 5}}


def imported_case(snapshot: object) -> str:
    if not isinstance(snapshot, dict) or type(snapshot.get("Version")) is not int or snapshot["Version"] != 1:
        raise ValueError("Expected a version 1 Godot case snapshot.")

    def read_enum_array(name: str, maximum: int) -> set[int]:
        values = snapshot.get(name)
        if not isinstance(values, list) or any(type(value) is not int or not 0 <= value <= maximum for value in values):
            raise ValueError(f"Invalid {name} list.")
        return set(values)

    sources = read_enum_array("Sources", 5)
    comparisons = read_enum_array("Comparisons", 2)
    challenged = snapshot.get("Challenged")
    if type(challenged) is not bool or (challenged and 1 not in comparisons):
        raise ValueError("Invalid witness challenge.")
    if any(not REQUIRED_SOURCES[comparison] <= sources for comparison in comparisons):
        raise ValueError("A comparison is missing its original sources.")

    observed = sum(1 << NATIVE_SOURCE[item] for item in sources if item in NATIVE_SOURCE)
    return f"2317-case-1 {observed} {int(challenged)} {int(0 in comparisons)}\n"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("godot_save", type=Path)
    parser.add_argument("native_save_directory", type=Path)
    args = parser.parse_args()
    path = args.native_save_directory / "case-apartment.txt"
    try:
        content = imported_case(json.loads(args.godot_save.read_text(encoding="utf-8")))
        if (args.native_save_directory / "case-progress.txt").exists():
            raise ValueError("The older native Apartment save already exists.")
        with path.open("x", encoding="ascii") as output:
            output.write(content)
    except (OSError, ValueError) as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
