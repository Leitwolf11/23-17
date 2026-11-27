# Architectural materials

The engine ships 512 px tiles derived from Poly Haven's 1K CC0 scans. The first five array layers store sRGB base color and linear ambient occlusion. The next five store tangent-space OpenGL normal XY, perceptual roughness and metalness in linear channels. SDL expands the PNG to RGBA8 before upload. The shader decodes base color separately from the data channels.

| Layer | Material |
| --- | --- |
| 0 | [Painted Plaster Wall](https://polyhaven.com/a/painted_plaster_wall) |
| 1 | [Floor Tiles 02](https://polyhaven.com/a/floor_tiles_02) |
| 2 | [Oak Wood Planks](https://polyhaven.com/a/oak_wood_planks) |
| 3 | [Asphalt 04](https://polyhaven.com/a/asphalt_04) |
| 4 | [Metal Plate 02](https://polyhaven.com/a/metal_plate_02) |

To repack, download each asset's 1K Diffuse, nor_gl and ARM JPG maps. Name each set `<asset>-color.jpg`, `<asset>-normal.jpg` and `<asset>-arm.jpg` in one directory. Run `python Build/PackMaterials.py <directory> Assets/Materials/surfaces.png` with Pillow installed. Builds use the committed atlas offline.

[Poly Haven asset license](https://polyhaven.com/license) · [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/)
