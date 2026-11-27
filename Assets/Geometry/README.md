# Case geometry

The original case props in this directory are released under [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) by The 23.17 Computer Game's Contributors.

The Apartment security monitor includes its curved CRT, molded surround, textured enclosure and an original CCTV image. Regenerate the self-contained asset with Blender.

```sh
blender -b --factory-startup --python-exit-code 1 --python Build/AuthorSecurityMonitor.py
```

For static models that use the textured import path, export glTF 2.0 as a GLB with indexed triangles, normals and UV set 0. Embed color, metallic/roughness, normal and occlusion textures as PNG images. Use opaque, single-sided materials and repeat wrapping. Bake UV node transforms into UV set 0. An emissive material can share the base color image and UVs and use an emissive factor proportional to the base color factor. Add models before the world bakes its reflection probes and static lighting.

The other case props use vertex colors and can be regenerated with `python3 Build/AuthorCaseProps.py`.
