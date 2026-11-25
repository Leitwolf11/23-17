#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Export the original case props as self-contained glTF binaries."""

import json
import math
from pathlib import Path
import struct


OUT = Path(__file__).resolve().parents[1] / "Assets" / "Geometry"


class Mesh:
    def __init__(self):
        self.positions = []
        self.colors = []

    def triangle(self, points, tint):
        a, b, c = points
        u, v = [b[i] - a[i] for i in range(3)], [c[i] - a[i] for i in range(3)]
        normal = (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])
        length = math.hypot(*normal)
        if length < 1e-8:
            return
        diffuse = abs(sum(axis * light for axis, light in zip(normal, (-0.37, 0.84, 0.39)))) / length
        color = tuple(channel * (0.60 + diffuse * 0.40) for channel in tint)
        self.positions.extend(points)
        self.colors.extend((color, color, color))

    def strip(self, start, end, tint):
        for index in range(len(start)):
            following = (index + 1) % len(start)
            self.triangle((start[index], start[following], end[following]), tint)
            self.triangle((start[index], end[following], end[index]), tint)

    def box(self, center, half, tint, chamfer=0.003):
        x, y, z = center
        a, b, c = half
        edge = min(chamfer, a * 0.75, b * 0.75, c * 0.75)

        def outline(height, width, depth):
            points = ((-width + edge, -depth), (width - edge, -depth),
                      (width, -depth + edge), (width, depth - edge),
                      (width - edge, depth), (-width + edge, depth),
                      (-width, depth - edge), (-width, -depth + edge))
            return [(x + dx, y + height, z + dz) for dx, dz in points]

        rings = (outline(-b, a - edge, c - edge), outline(-b + edge, a, c),
                 outline(b - edge, a, c), outline(b, a - edge, c - edge))
        for lower, upper in zip(rings, rings[1:]):
            self.strip(lower, upper, tint)
        for cap in (rings[0], rings[-1]):
            for index in range(1, len(cap) - 1):
                self.triangle((cap[0], cap[index], cap[index + 1]), tint)

    def cylinder(self, center, radius, half, tint, axis="z", segments=24):
        def ring(depth):
            result = []
            for index in range(segments):
                angle = math.tau * index / segments
                x, y = math.cos(angle) * radius, math.sin(angle) * radius
                vector = (x, depth, y) if axis == "y" else (x, y, depth)
                result.append(tuple(a + b for a, b in zip(center, vector)))
            return result

        start, end = ring(-half), ring(half)
        self.strip(start, end, tint)
        for cap in (start, end):
            for index in range(1, len(cap) - 1):
                self.triangle((cap[0], cap[index], cap[index + 1]), tint)

    def export(self, name):
        def pack(values):
            return b"".join(struct.pack("<3f", *value) for value in values)

        positions = pack(self.positions)
        colors = pack(self.colors)
        model = {
            "asset": {"version": "2.0", "copyright": "The 23.17 Computer Game's Contributors, CC0 1.0"},
            "scene": 0,
            "scenes": [{"nodes": [0]}],
            "nodes": [{"mesh": 0, "name": name}],
            "meshes": [{"primitives": [{"attributes": {"POSITION": 0, "COLOR_0": 1}}]}],
            "buffers": [{"byteLength": len(positions) + len(colors)}],
            "bufferViews": [{"buffer": 0, "byteLength": len(positions), "target": 34962},
                            {"buffer": 0, "byteOffset": len(positions), "byteLength": len(colors), "target": 34962}],
            "accessors": [
                {"bufferView": 0, "componentType": 5126, "count": len(self.positions), "type": "VEC3",
                 "min": [min(axis) for axis in zip(*self.positions)], "max": [max(axis) for axis in zip(*self.positions)]},
                {"bufferView": 1, "componentType": 5126, "count": len(self.colors), "type": "VEC3"}
            ],
        }
        metadata = json.dumps(model, separators=(",", ":")).encode("ascii")
        metadata += b" " * (-len(metadata) % 4)
        buffer = positions + colors
        header = struct.pack("<4sII", b"glTF", 2, 28 + len(metadata) + len(buffer))
        (OUT / f"{name}.glb").write_bytes(header + struct.pack("<I4s", len(metadata), b"JSON")
                                         + metadata + struct.pack("<I4s", len(buffer), b"BIN\0") + buffer)


def stair_recorder():
    mesh = Mesh()
    steel = (0.24, 0.28, 0.31)
    black = (0.12, 0.13, 0.14)
    mesh.box((0, 0, 0), (0.18, 0.125, 0.21), steel, 0.022)
    mesh.box((-0.174, 0, 0), (0.018, 0.113, 0.196), (0.36, 0.38, 0.37), 0.009)
    mesh.box((-0.193, 0.033, -0.055), (0.007, 0.042, 0.095), black)
    mesh.box((-0.200, 0.035, -0.055), (0.002, 0.027, 0.075), (0.25, 0.48, 0.39))
    for row in range(4):
        for col in range(5):
            mesh.box((-0.196, -0.067 + row * 0.017, 0.058 + col * 0.024),
                     (0.004, 0.004, 0.006), black)
    for z in (-0.14, -0.087, -0.035):
        mesh.box((-0.193, -0.061, z), (0.013, 0.018, 0.021), (0.49, 0.50, 0.48))
    for z in (-0.157, 0.157):
        mesh.box((0.021, -0.142, z), (0.125, 0.028, 0.026), black)
    mesh.export("stair-recorder")


def desk_telephone():
    mesh = Mesh()
    body = (0.27, 0.24, 0.19)
    handset = (0.18, 0.16, 0.13)
    dial = (0.50, 0.46, 0.34)
    mesh.box((0, 0.04, 0), (0.143, 0.045, 0.126), body, 0.024)
    mesh.cylinder((0.024, 0.088, 0.030), 0.065, 0.009, dial, axis="y")
    for index in range(10):
        angle = math.tau * index / 10
        mesh.cylinder((0.024 + 0.050 * math.cos(angle), 0.098, 0.03 + 0.050 * math.sin(angle)),
                      0.009, 0.003, handset, axis="y", segments=10)
    mesh.cylinder((0.024, 0.099, 0.03), 0.026, 0.004, (0.68, 0.66, 0.52), axis="y")
    mesh.box((0, 0.114, -0.084), (0.112, 0.023, 0.025), handset, 0.014)
    for x in (-0.104, 0.104):
        mesh.cylinder((x, 0.13, -0.079), 0.044, 0.021, handset, axis="y")
    mesh.export("desk-telephone")


def projector():
    mesh = Mesh()
    black = (0.18, 0.18, 0.17)
    enamel = (0.33, 0.36, 0.32)
    metal = (0.54, 0.53, 0.44)
    mesh.box((0, 0.19, 0), (0.22, 0.19, 0.17), enamel, 0.030)
    mesh.box((0, 0.39, -0.02), (0.14, 0.085, 0.14), black, 0.020)
    mesh.cylinder((0, 0.29, -0.205), 0.084, 0.070, black)
    mesh.cylinder((0, 0.29, -0.278), 0.061, 0.011, metal)
    mesh.cylinder((0, 0.29, -0.292), 0.041, 0.004, (0.24, 0.46, 0.48))
    for x in (-0.16, 0.16):
        for y in (0.51, 0.72):
            mesh.box((x, y - 0.105, 0.035), (0.026, 0.108, 0.028), enamel)
        for center_z in (0.042, 0.084):
            mesh.cylinder((x, 0.69, center_z), 0.137, 0.006, metal)
        mesh.cylinder((x, 0.69, 0.063), 0.125, 0.030, black)
        for index in range(5):
            angle = math.tau * index / 5
            mesh.cylinder((x + 0.089 * math.cos(angle), 0.69 + 0.089 * math.sin(angle), 0.092),
                          0.028, 0.003, black, segments=12)
        mesh.cylinder((x, 0.69, 0.102), 0.027, 0.021, metal)
    for slot in range(8):
        mesh.box((0.222, 0.12 + slot * 0.022, 0.035), (0.006, 0.005, 0.08), black)
    mesh.export("projector")


if __name__ == "__main__":
    OUT.mkdir(parents=True, exist_ok=True)
    stair_recorder()
    desk_telephone()
    projector()
