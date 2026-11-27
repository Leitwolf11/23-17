# Case geometry

The original case props in this directory are released under [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) by The 23.17 Computer Game's Contributors.

The Apartment security monitor includes its curved CRT, molded surround, textured enclosure and an original CCTV image. Regenerate the self-contained asset with Blender.

```sh
blender -b --factory-startup --python-exit-code 1 --python Build/AuthorSecurityMonitor.py
```

The Apartment joinery includes the writing desk, security cabinet, clipboard, service-exit reader, and stair handrail. The oak and iron finishes are derived from the CC0 scans already bundled with the [architectural material atlas](../Materials/README.md). The GLB uses the Apartment's origin and places the meshes directly in the scene.

```sh
blender -b --factory-startup --python-exit-code 1 --python Build/AuthorApartmentJoinery.py
```

For static models that use the textured import path, export glTF 2.0 as a GLB with indexed triangles, normals and UV set 0. Embed color, metallic/roughness, normal and occlusion textures as PNG images. Use opaque, single-sided materials and repeat wrapping. Bake UV node transforms into UV set 0. An emissive material can share the base color image and UVs and use an emissive factor proportional to the base color factor. Low uses a representative base color from the imported texture when detail sampling is disabled. Add models before the world bakes its reflection probes and static lighting.

The other case props use vertex colors and can be regenerated with `python3 Build/AuthorCaseProps.py`.
