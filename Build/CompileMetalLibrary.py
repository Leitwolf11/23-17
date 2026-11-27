# SPDX-License-Identifier: MIT
"""Compile the six renderer stages with the macOS SDK and embed the Metal 3 library."""
import sys
from pathlib import Path
from subprocess import run  # nosec B404
from tempfile import TemporaryDirectory

output, *sources = map(Path, sys.argv[1:])
with TemporaryDirectory() as temp:
    root = Path(temp)
    objects = []
    for source in sources:
        obj = root / (source.stem + ".air")
        # Bazel supplies the checked-in shader paths as separate arguments.
        run(["xcrun",  # nosec B603, B607
             "--sdk", "macosx", "metal", "-std=metal3.0", "-mmacosx-version-min=13.0", "-c",
             str(source), "-o", str(obj)], check=True)
        objects.append(obj)
    library = root / "Scene.metallib"
    run(["xcrun", "--sdk", "macosx", "metallib",  # nosec B603, B607
         *map(str, objects),
         "-o", str(library)], check=True)
    data = library.read_bytes()
    lines = ["alignas(16) constexpr unsigned char kMetalLibrary[] = {"]
    for offset in range(0, len(data), 24):
        lines.append(",".join(map(str, data[offset:offset + 24])) + ",")
    output.write_text("\n".join([*lines, "};", ""]), encoding="utf-8")
