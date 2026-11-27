# SPDX-License-Identifier: MIT
"""Author the Apartment entrance joinery and fittings as a metric Blender GLB."""

from array import array
from pathlib import Path
import math
import sys
import tempfile

import bpy

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from AuthorSecurityMonitor import GLYPHS, box, image_node, material, mesh, png, tube


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "Assets/Geometry/apartment-joinery.glb"
TILE_SIZE = 512


def extract_finish(atlas, directory, name, layer, patina):
    """Repack an existing architectural scan into ordinary glTF PNG inputs."""
    color, normal, metallic_roughness = bytearray(), bytearray(), bytearray()
    for row in range(TILE_SIZE):
        color_row = (TILE_SIZE * (10 - layer) - 1 - row) * TILE_SIZE * 4
        data_row = color_row - TILE_SIZE * TILE_SIZE * 4 * 5
        for column in range(TILE_SIZE):
            color_index = color_row + 4 * column
            data_index = data_row + 4 * column
            color.extend(max(0, min(255, round(atlas[color_index + c] * 255 * patina[c] + 6)))
                         for c in range(3))
            normal_x, normal_y, roughness, metallic = atlas[data_index:data_index + 4]
            normal_z = math.sqrt(max(0, 1 - (2 * normal_x - 1)**2 - (2 * normal_y - 1)**2))
            normal.extend((round(normal_x * 255), round(normal_y * 255), round((.5 + .5 * normal_z) * 255)))
            metallic_roughness.extend((round(atlas[color_index + 3] * 255),
                                       round((.28 + .66 * roughness) * 255),
                                       round(metallic * 255)))
    for channel, pixels in (("color", color), ("normal", normal), ("orm", metallic_roughness)):
        png(directory / f"{name}-{channel}.png", TILE_SIZE, TILE_SIZE, pixels)


def scanned_finish(directory, name, label):
    item, shader = material(label, (.26, .22, .17), .59)
    nodes, links = item.node_tree.nodes, item.node_tree.links
    links.new(image_node(item, directory / f"{name}-color.png").outputs["Color"], shader.inputs["Base Color"])
    normal = nodes.new("ShaderNodeNormalMap")
    normal.inputs["Strength"].default_value = .42
    links.new(image_node(item, directory / f"{name}-normal.png", True).outputs["Color"], normal.inputs["Color"])
    links.new(normal.outputs["Normal"], shader.inputs["Normal"])
    packed = image_node(item, directory / f"{name}-orm.png", True)
    channels = nodes.new("ShaderNodeSeparateColor")
    links.new(packed.outputs["Color"], channels.inputs[0])
    links.new(channels.outputs["Green"], shader.inputs["Roughness"])
    links.new(channels.outputs["Blue"], shader.inputs["Metallic"])
    group = bpy.data.node_groups.get("glTF Material Output")
    if group is None:
        group = bpy.data.node_groups.new("glTF Material Output", "ShaderNodeTree")
        group.interface.new_socket(name="Occlusion", in_out="INPUT", socket_type="NodeSocketFloat")
    settings = nodes.new("ShaderNodeGroup")
    settings.node_tree = group
    links.new(packed.outputs["Color"], settings.inputs["Occlusion"])
    return item


def plank(name, center, size, finish, offset=0.0, bevel=.008):
    obj = box(name, center, size, bevel, finish)
    obj.data.update()
    layer = obj.data.uv_layers.active
    for face in obj.data.polygons:
        for index in face.loop_indices:
            point = obj.data.vertices[obj.data.loops[index].vertex_index].co
            if abs(face.normal.z) > .65:
                u, v = point.y / .80, point.x / .96
            elif abs(face.normal.y) > .65:
                u, v = point.x / .80, point.z / .96
            else:
                u, v = point.y / .80, point.z / .96
            layer.data[index].uv = (u + offset, v + .37 + .61 * offset)
    return obj


def writing_desk(wood, metal):
    x, y = -1.50, -2.18
    plank("Writing desk bullnose top", (x, y, .851), (.78, .92, .068), wood, .27, .017)
    for i, side in enumerate((-1, 1)):
        for j, end in enumerate((-1, 1)):
            leg = plank("Tapered writing desk leg", (x + side * .30, y + end * .365, .39),
                        (.064, .066, .77), wood, .14 * (i * 2 + j))
            for vertex in leg.data.vertices:
                t = max(0, min(1, (vertex.co.z + .385) / .77))
                vertex.co.x = vertex.co.x * (.62 + .38 * t) + side * .024 * (1 - t)
                vertex.co.y = vertex.co.y * (.62 + .38 * t) + end * .021 * (1 - t)
        plank("Side apron", (x + side * .30, y, .757), (.040, .709, .125), wood, .22)
        plank("Bottom stretcher", (x + side * .30, y, .194), (.047, .725, .045), wood, .53)
    plank("Apron", (x, y + .362, .769), (.592, .029, .096), wood, .11)
    plank("Inset drawer face", (x, y - .379, .766), (.639, .032, .105), wood, .63)
    tube("Drawer pull", [(x - .073, y - .400, .767), (x - .056, y - .426, .742),
                         (x + .056, y - .426, .742), (x + .073, y - .400, .767)],
         .007, metal, 2)


def security_cabinet(wood, metal):
    x, y = 1.50, -2.18
    plank("Console rounded top", (x, y, .847), (.78, .92, .076), wood, .15, .019)
    plank("Console plinth", (x, y, .061), (.700, .819, .112), wood, .80)
    plank("Console back", (x, y + .386, .446), (.677, .040, .711), wood, .17)
    for side in (-1, 1):
        plank("Console side", (x + .327 * side, y, .449), (.042, .788, .718), wood, .31 * side)
        plank("Framed cabinet door", (x + .153 * side, y - .381, .444),
              (.299, .028, .608), wood, .65 + .32 * side, .010)
        plank("Cabinet raised panel", (x + .153 * side, y - .396, .444),
              (.221, .016, .506), wood, .24 * side, .012)
        tube("Cabinet pull", [(x + .072 * side, y - .409, .520),
                              (x + .072 * side, y - .433, .508),
                              (x + .072 * side, y - .433, .405),
                              (x + .072 * side, y - .409, .393)], .006, metal, 2)
    plank("Console drawer", (x, y - .381, .762), (.626, .033, .059), wood, .15)


def stair_rail(wood, iron, brass):
    start_z, end_z = 1.06, 6.34
    start_height, end_height = 1.03, 3.38

    def rail_height(depth):
        return start_height + (end_height - start_height) * (depth - start_z) / (end_z - start_z)

    top = [(1.77, 1.01, 1.006), (1.77, 1.029, 1.019),
           (1.77, start_z, start_height), (1.77, end_z, end_height),
           (1.77, 6.374, 3.383), (1.796, 6.395, 3.371),
           (1.840, 6.408, 3.344)]
    grip = tube("Rounded timber stair rail", top, .041, wood, 3)
    for polygon in grip.data.polygons:
        polygon.use_smooth = True
    uv = grip.data.uv_layers.active
    for loop in grip.data.loops:
        point = grip.data.vertices[loop.vertex_index].co
        angle = math.atan2(point.z - rail_height(point.y), point.x - 1.77)
        uv.data[loop.index].uv = (angle / math.tau + .5, point.y / 2.4)

    for step in range(0, 15, 3):
        depth = 1.19 + step * .36
        landing = .16 * (step + 1)
        x, top_height = 1.77, rail_height(depth)
        box("Stair rail shoe", (x, depth, landing + .014), (.115, .087, .028), .009, iron)
        post = tube("Stair baluster", [(x, depth, landing + .023),
                                       (x, depth, top_height - .047)], .024, iron, 2)
        for polygon in post.data.polygons:
            polygon.use_smooth = True
        for y_angle in (-1, 1):
            tube("Baluster fixing", [(x, depth + y_angle * .029, landing + .021),
                                      (x, depth + y_angle * .029, landing + .031)],
                 .007, brass, 1)
        tube("Saddle at grip", [(x, depth, top_height - .058),
                                  (x, depth, top_height - .036)],
             .030, brass, 1)


def dispatch_texture(path):
    width, height = 320, 224
    rgb = bytearray()
    for row in range(height):
        for col in range(width):
            fiber = (col * 73 + row * 151 + col * row * 13) % 19 - 9
            edge = min(col, row, width - col - 1, height - row - 1)
            value = 222 + fiber // 2 - max(0, 9 - edge)
            rule = row in range(82, 193, 24) or (row == 55 and 25 < col < 294)
            if rule and 29 < col < 291:
                rgb.extend((123, 128, 119))
            else:
                rgb.extend((value, value - 10, value - 31))

    for index, glyph in enumerate("23:14"):
        pattern = GLYPHS[glyph] if glyph != "4" else (18, 18, 18, 31, 2, 2, 2)
        for row, line in enumerate(pattern):
            for col in range(5):
                if line & (16 >> col):
                    for dy in range(3):
                        for dx in range(3):
                            x, y = 32 + index * 19 + col * 3 + dx, 22 + row * 3 + dy
                            pixel = (y * width + x) * 3
                            rgb[pixel:pixel + 3] = bytes((55, 49, 44))
    png(path, width, height, rgb)


def dispatch_sheet(wood, brass, directory):
    texture = directory / "dispatch-sheet.png"
    dispatch_texture(texture)
    paper, shader = material("Printed dispatch sheet", (.66, .59, .43), .85)
    paper.node_tree.links.new(image_node(paper, texture).outputs["Color"], shader.inputs["Base Color"])
    plank("Dispatch clipboard", (-1.50, -2.20, .894), (.386, .283, .018), wood, .43, .007)
    box("Clipboard spring clip", (-1.50, -2.105, .926), (.166, .038, .019), .008, brass)

    rows, columns = 8, 12
    points, uv, faces = [], [], []
    for row in range(rows + 1):
        for column in range(columns + 1):
            u, v = column / columns, row / rows
            curl = .002 * math.sin(math.pi * u) + .014 * u**8 * (1 - v)**5
            points.append((-1.679 + .358 * u, -2.332 + .254 * v, .906 + curl))
            uv.append((u, v))
    for row in range(rows):
        for column in range(columns):
            index = row * (columns + 1) + column
            faces.append((index, index + 1, index + columns + 2, index + columns + 1))
    mesh("Curled dispatch paper", points, faces, uv, paper)


def exit_reader(iron, brass, dial):
    # Blender negative Y corresponds to positive Z at the bottom of the stairs.
    x, y = -1.85, -.12
    plank("Service exit reader housing", (x, y, 1.40), (.198, .456, .389), iron, .28, .018)
    plank("Reader inset bezel", (-1.743, y, 1.444), (.034, .394, .272), iron, .74, .013)
    box("Reader status window", (-1.722, y, 1.495), (.008, .252, .107), .004, dial)
    box("Service plate", (-1.72, y + .054, 1.329), (.005, .129, .037), .004, brass)
    for y_offset in (-.167, .167):
        for height in (1.337, 1.550):
            tube("Reader cap screw", [(-1.727, y + y_offset, height),
                                      (-1.718, y + y_offset, height)], .007, brass, 1)
    tube("Reader steel conduit", [(-1.94, y + .058, 1.207),
                                   (-1.945, y + .058, .648),
                                   (-1.958, y + .093, .578)], .013, iron, 2)


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    with tempfile.TemporaryDirectory() as staging:
        directory = Path(staging)
        atlas = bpy.data.images.load(str(ROOT / "Assets/Materials/surfaces.png"))
        atlas.colorspace_settings.name = "Non-Color"
        assert tuple(atlas.size) == (TILE_SIZE, 10 * TILE_SIZE)
        pixels = array("f", [0]) * len(atlas.pixels)
        atlas.pixels.foreach_get(pixels)
        extract_finish(pixels, directory, "oak", 2, (.68, .66, .64))
        extract_finish(pixels, directory, "iron", 4, (.56, .61, .64))
        bpy.data.images.remove(atlas)
        wood = scanned_finish(directory, "oak", "Worn oiled oak")
        iron = scanned_finish(directory, "iron", "Patinated dark iron")
        brass, _ = material("Tarnished brass fittings", (.34, .22, .072), .41, .78)
        dial_color = (.018, .080, .052)
        dial, shader = material("Green reader glass", dial_color, .23)
        shader.inputs["Emission Color"].default_value = (*dial_color, 1)
        shader.inputs["Emission Strength"].default_value = .8

        writing_desk(wood, brass)
        security_cabinet(wood, brass)
        stair_rail(wood, iron, brass)
        dispatch_sheet(wood, brass, directory)
        exit_reader(iron, brass, dial)
        bpy.context.scene["copyright"] = "The 23.17 Computer Game's Contributors, CC0 1.0"
        bpy.ops.export_scene.gltf(filepath=str(OUTPUT), export_format="GLB",
                                  export_image_format="AUTO", export_materials="EXPORT",
                                  export_apply=True, export_animations=False,
                                  export_yup=True, export_normals=True,
                                  export_texcoords=True, export_skins=False,
                                  export_copyright="The 23.17 Computer Game's Contributors, CC0 1.0")


if __name__ == "__main__":
    main()
