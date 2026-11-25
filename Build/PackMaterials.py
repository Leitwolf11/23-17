# SPDX-License-Identifier: MIT
"""Pack the CC0 1K source maps listed in Assets/Materials/README.md."""

from argparse import ArgumentParser
from pathlib import Path

from PIL import Image

names = ("painted_plaster_wall", "floor_tiles_02", "oak_wood_planks", "asphalt_04", "metal_plate_02")
parser = ArgumentParser(description=__doc__)
parser.add_argument("directory", type=Path)
parser.add_argument("output", type=Path)
parser.add_argument("--reference", action="store_true", help="Pack the scanned reference objects.")
args = parser.parse_args()
if args.reference:
    names = ("marble_bust_01", "brass_vase_01")
size = 512
atlas = Image.new("RGBA", (size, size * 2 * len(names)))
for index, name in enumerate(names):
    def read(kind):
        path = args.directory / f"{name}-{kind}.jpg"
        with Image.open(path) as image:
            return image.convert("RGB").resize((size, size), Image.Resampling.LANCZOS).split()

    albedo = read("color")
    nx, ny, _ = read("normal")
    ao, roughness, metal = read("arm")
    atlas.paste(Image.merge("RGBA", (*albedo, ao)), (0, index * size))
    atlas.paste(Image.merge("RGBA", (nx, ny, roughness, metal)), (0, (index + len(names)) * size))
atlas.save(args.output)
