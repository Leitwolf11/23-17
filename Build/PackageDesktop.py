#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Package a built desktop executable and its runtime assets."""

import argparse
from pathlib import Path
from tempfile import TemporaryDirectory
from zipfile import ZIP_DEFLATED, ZipFile


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("archive", type=Path)
    parser.add_argument("--version", help="Release identifier stored beside the executable.")
    parser.add_argument("--runtime", type=Path, action="append", default=[])
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    legacy_assets = root / "LegacyDemo" / "assets"
    recordings = sorted((legacy_assets / "audio").glob("*.wav"))
    meshes = sorted((root / "Assets" / "Geometry").glob("*.glb"))
    if not args.executable.is_file():
        parser.error(f"Missing executable: {args.executable}")
    if not meshes or not recordings:
        parser.error("The source checkout is missing case assets.")
    for dependency in args.runtime:
        if not dependency.is_file():
            parser.error(f"Missing runtime dependency: {dependency}")

    args.archive.parent.mkdir(parents=True, exist_ok=True)
    with TemporaryDirectory(dir=args.archive.parent) as directory:
        staged = Path(directory) / args.archive.name
        with ZipFile(staged, "w", compression=ZIP_DEFLATED) as archive:
            for file in [args.executable, *args.runtime]:
                archive.write(file.resolve(), f"23.17/{file.name}")
            if args.version:
                archive.writestr("23.17/VERSION.txt", args.version + "\n")
            archive.write(root / "LICENSE", "23.17/LICENSE")
            archive.write(legacy_assets / "LICENSE", "23.17/audio/LICENSE")
            for license_file in sorted((root / "Docs" / "ThirdPartyLicenses").glob("*.txt")):
                archive.write(license_file, f"23.17/licenses/{license_file.name}")
            for mesh in meshes:
                archive.write(mesh, f"23.17/geometry/{mesh.name}")
            for recording in recordings:
                archive.write(recording, f"23.17/audio/{recording.name}")
        staged.replace(args.archive)


if __name__ == "__main__":
    main()
