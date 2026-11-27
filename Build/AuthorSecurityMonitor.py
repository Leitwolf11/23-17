# SPDX-License-Identifier: MIT
"""Build the original CRT evidence model with Blender and export a packed GLB."""

from pathlib import Path
import math
import random
import struct
import tempfile
import zlib

import bpy
import bmesh


OUTPUT = Path(__file__).resolve().parents[1] / "Assets/Geometry/security-monitor.glb"


def png(path, width, height, rgb):
    def chunk(kind, payload):
        return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload))

    row = width * 3
    data = b"".join(b"\0" + rgb[start:start + row] for start in range(0, len(rgb), row))
    header = struct.pack(">2I5B", width, height, 8, 2, 0, 0, 0)
    path.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header)
                     + chunk(b"IDAT", zlib.compress(data)) + chunk(b"IEND", b""))


GLYPHS = {
    "A": (14, 17, 17, 31, 17, 17, 17), "C": (14, 17, 16, 16, 16, 17, 14),
    "M": (17, 27, 21, 21, 17, 17, 17), "R": (30, 17, 17, 30, 20, 18, 17),
    "E": (31, 16, 16, 30, 16, 16, 31), "0": (14, 17, 19, 21, 25, 17, 14),
    "1": (4, 12, 4, 4, 4, 4, 14), "2": (14, 17, 1, 6, 8, 16, 31),
    "3": (30, 1, 1, 14, 1, 1, 30), "7": (31, 1, 2, 4, 8, 8, 8),
    ":": (0, 4, 4, 0, 4, 4, 0),
}


def display_image(path):
    width, height = 384, 288
    rgb = bytearray()
    rng = random.Random(2317)  # nosec B311 - repeatable image grain
    for y in range(height):
        for x in range(width):
            u, v = x / width, y / height
            falloff = max(0.25, 1 - 1.5 * ((u - .5)**2 + (v - .5)**2))
            # An original, grainy monochrome view down a service passage.
            tunnel = abs(u - .57) < .15 + .65 * abs(v - .48)
            lamp = math.exp(-120 * (u - .57)**2 - 440 * (v - .31)**2)
            doorway = abs(u - .57) < .085 and .38 < v < .67
            steps = v > .60 and int(10 * math.sqrt(v - .58)) % 2 == 0
            tile = int((u - .57) * 9 / max(v - .50, .12)) % 2 == 0
            brightness = ((46 if tunnel else 21) + 74 * lamp
                          - 25 * doorway + v * 14 + (8 * tile - 13 * steps if v > .60 else 0))
            brightness = (brightness + rng.uniform(-10, 10)) * falloff * (.82 if y % 3 == 0 else 1)
            rgb.extend(int(max(0, min(255, brightness * c))) for c in (.81, 1.19, 1.10))

    def put(x, y, tint):
        index = (y * width + x) * 3
        rgb[index:index + 3] = bytes(tint)

    def label(text, x, y, tint):
        for index, symbol in enumerate(text):
            for row, bits in enumerate(GLYPHS.get(symbol, (0,) * 7)):
                for column in range(5):
                    if bits & (16 >> column):
                        for dy in range(2):
                            for dx in range(2):
                                put(x + index * 12 + column * 2 + dx, y + row * 2 + dy, tint)

    label("CAM 02", 26, 25, (176, 211, 200))
    label("REC", 306, 25, (207, 117, 102))
    label("23:17:02", 26, 245, (176, 211, 200))
    for x0, direction in ((21, 1), (361, -1)):
        for y0, vertical in ((49, 1), (227, -1)):
            for i in range(15):
                put(x0 + direction * i, y0, (108, 153, 141))
                put(x0, y0 + vertical * i, (108, 153, 141))
    png(path, width, height, rgb)


def case_maps(directory):
    size = 128
    rng = random.Random(507)  # nosec B311 - repeatable surface grain
    height = [rng.uniform(-1, 1) for _ in range(size * size)]
    color, normal, surface, occlusion = bytearray(), bytearray(), bytearray(), bytearray()
    for y in range(size):
        for x in range(size):
            def sample(dx, dy):
                return height[((y + dy) % size) * size + ((x + dx) % size)]

            n = sample(0, 0)
            gray = 112 + int(9 * n + 5 * math.sin(.2 * x + .42 * y))
            color.extend((gray + 5, gray + 2, gray - 4))
            normal.extend((128 + int(15 * (sample(-1, 0) - sample(1, 0))),
                           128 + int(15 * (sample(0, 1) - sample(0, -1))), 254))
            surface.extend((255, int(170 + 32 * n), int(90 + 25 * n)))
            occlusion.extend((int(242 + 11 * n), 255, 255))
    for name, data in (("finish", color), ("normal", normal), ("surface", surface), ("occlusion", occlusion)):
        png(directory / (name + ".png"), size, size, data)


def material(name, color, roughness, metallic=0):
    entry = bpy.data.materials.new(name)
    entry.diffuse_color = (*color, 1)
    entry.use_backface_culling = True
    shader = entry.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (*color, 1)
    shader.inputs["Roughness"].default_value = roughness
    shader.inputs["Metallic"].default_value = metallic
    shader.inputs["Emission Strength"].default_value = 0
    return entry, shader


def image_node(entry, path, linear=False):
    node = entry.node_tree.nodes.new("ShaderNodeTexImage")
    node.image = bpy.data.images.load(str(path))
    node.extension = "REPEAT"
    node.image.colorspace_settings.name = "Non-Color" if linear else "sRGB"
    node.image.pack()
    return node


def finish(directory):
    entry, shader = material("Textured enamel", (.72, .75, .76), .83, .7)
    nodes, links = entry.node_tree.nodes, entry.node_tree.links
    links.new(image_node(entry, directory / "finish.png").outputs["Color"], shader.inputs["Base Color"])
    normal = nodes.new("ShaderNodeNormalMap")
    links.new(image_node(entry, directory / "normal.png", True).outputs["Color"], normal.inputs["Color"])
    links.new(normal.outputs["Normal"], shader.inputs["Normal"])
    separate = nodes.new("ShaderNodeSeparateColor")
    links.new(image_node(entry, directory / "surface.png", True).outputs["Color"], separate.inputs[0])
    links.new(separate.outputs["Green"], shader.inputs["Roughness"])
    links.new(separate.outputs["Blue"], shader.inputs["Metallic"])
    group = bpy.data.node_groups.new("glTF Material Output", 'ShaderNodeTree')
    group.interface.new_socket(name="Occlusion", in_out="INPUT", socket_type="NodeSocketFloat")
    output = nodes.new("ShaderNodeGroup")
    output.node_tree = group
    links.new(image_node(entry, directory / "occlusion.png", True).outputs["Color"], output.inputs["Occlusion"])
    return entry


def box(name, center, dimensions, radius, finish):
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    bevel = obj.modifiers.new("Molded edges", "BEVEL")
    bevel.width, bevel.segments = radius, 3 if radius > .02 else 2
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    obj.modifiers.new("Face weighted normals", "WEIGHTED_NORMAL")
    obj.data.materials.append(finish)
    return obj


def tube(name, points, radius, finish, resolution=2):
    curve = bpy.data.curves.new(name, "CURVE")
    curve.dimensions, curve.bevel_depth, curve.bevel_resolution = "3D", radius, resolution
    curve.use_fill_caps = True
    spline = curve.splines.new("POLY")
    spline.points.add(len(points) - 1)
    for sample, (x, y, z) in zip(spline.points, points):
        sample.co = (x, y, z, 1)
    obj = bpy.data.objects.new(name, curve)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(finish)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.ops.object.convert(target="MESH")
    if not obj.data.uv_layers:
        obj.data.uv_layers.new()
    return obj


def mesh(name, positions, faces, coordinates, finish):
    raw = bpy.data.meshes.new(name)
    raw.from_pydata(positions, [], faces)
    raw.update()
    uv = raw.uv_layers.new()
    for loop in raw.loops:
        uv.data[loop.index].uv = coordinates[loop.vertex_index]
    for face in raw.polygons:
        face.use_smooth = True
    obj = bpy.data.objects.new(name, raw)
    bpy.context.collection.objects.link(obj)
    raw.materials.append(finish)
    return obj


def bezel(finish):
    vertices, uvs, faces = [], [], []
    # Rounded rectangle outlines form a stepped molding around the tube.
    for width, height, radius, depth in (
        (.188, .142, .032, -.118),
        (.189, .143, .031, -.151),
        (.177, .132, .026, -.169),
        (.152, .111, .023, -.172),
        (.144, .105, .022, -.152),
    ):
        for corner in range(4):
            center_angle = corner * math.pi / 2
            cx = math.copysign(width - radius, math.cos(center_angle + math.pi / 4))
            cz = math.copysign(height - radius, math.sin(center_angle + math.pi / 4))
            for segment in range(8):
                angle = center_angle + segment * math.pi / 14
                x, z = cx + radius * math.cos(angle), cz + radius * math.sin(angle)
                vertices.append((x - .023, depth, .209 + z))
                uvs.append(((x / .4) + .5, (z / .34) + .5))
    for ring in range(4):
        for index in range(32):
            next_index = (index + 1) % 32
            first, second = ring * 32, (ring + 1) * 32
            faces.append((first + index, first + next_index, second + next_index, second + index))
    obj = mesh("Rounded stepped bezel", vertices, faces, uvs, finish)
    # Orient the visible surround toward the front of the monitor.
    editable = bmesh.new()
    editable.from_mesh(obj.data)
    reference = min(editable.faces, key=lambda face: abs(face.calc_center_median().y + .170))
    if reference.normal.y > 0:
        bmesh.ops.reverse_faces(editable, faces=list(editable.faces))
    editable.to_mesh(obj.data)
    editable.free()


def glass(finish):
    vertices, uvs, faces = [], [], []
    steps = 16
    for row in range(steps + 1):
        for col in range(steps + 1):
            u, v = 2 * col / steps - 1, 2 * row / steps - 1
            x = -.023 + .153 * u * math.sqrt(1 - .15 * v**12)
            z = .209 + .112 * v * math.sqrt(1 - .14 * u**12)
            y = -.150 - .014 * (1 - .64 * u * u) * (1 - .71 * v * v)
            vertices.append((x, y, z))
            uvs.append((col / steps, row / steps))
    for row in range(steps):
        for col in range(steps):
            index = row * (steps + 1) + col
            faces.append((index, index + 1, index + steps + 2, index + steps + 1))
    mesh("Curved glass tube", vertices, faces, uvs, finish)


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    with tempfile.TemporaryDirectory() as staging:
        directory = Path(staging)
        display_image(directory / "security-feed.png")
        case_maps(directory)
        enamel = finish(directory)
        polymer, _ = material("Dark graphite polymer", (.045, .051, .053), .76)
        inset, _ = material("Rubber and ventilation", (.012, .016, .016), .88)
        metal, _ = material("Brushed hardware", (.37, .40, .40), .33, .87)
        glass_material, shader = material("Phosphor display", (.45, .45, .45), .19)
        texture = image_node(glass_material, directory / "security-feed.png")
        glass_material.node_tree.links.new(texture.outputs["Color"], shader.inputs["Base Color"])
        glass_material.node_tree.links.new(texture.outputs["Color"], shader.inputs["Emission Color"])
        shader.inputs["Emission Strength"].default_value = .77

        box("Front chassis", (0, -.018, .192), (.445, .246, .351), .032, enamel)
        box("Tapered rear case", (0, .136, .192), (.407, .172, .317), .047, enamel)
        bezel(polymer)
        glass(glass_material)
        box("Service label", (.171, -.143, .115), (.049, .006, .012), .002, metal)
        for index in range(6):
            box("Speaker grille recess", (.171, -.144, .209 + index * .011),
                (.043, .007, .0038), .0015, inset)
        for index in range(3):
            box("Menu key", (.140 + index * .019, -.144, .087),
                (.011, .013, .008), .0025, polymer)
        box("Power key", (.178, -.144, .063), (.030, .014, .013), .004, metal)
        for side in (-1, 1):
            box("Rubber foot", (side * .146, .022, .022), (.080, .146, .044), .009, inset)
            for index in range(5):
                box("Rear cooling recess", (side * (.177 - index * .031), .087, .354),
                    (.012, .137, .0045), .0022, inset)
        tube("Power lead", [(x, y, z) for x, y, z in (
            (.096, .209, .120), (.096, .241, .111), (.086, .264, .078),
            (.052, .286, .038), (.010, .316, .021), (-.053, .354, .008),
        )], .005, inset)
        bpy.context.scene["copyright"] = "The 23.17 Computer Game's Contributors, CC0 1.0"
        bpy.ops.export_scene.gltf(filepath=str(OUTPUT), export_format="GLB",
                                  export_image_format="AUTO", export_materials="EXPORT",
                                  export_apply=True, export_animations=False,
                                  export_yup=True, export_normals=True,
                                  export_texcoords=True, export_skins=False, export_extras=True)


if __name__ == "__main__":
    main()
