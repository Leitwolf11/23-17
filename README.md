# 23.17

23.17 is a single-player, first-person 3D detective horror anthology. Examine original evidence, challenge testimony, reconstruct the timeline and file a supported report.

## Development status

The native build opens three playable investigations. **The Apartment** follows a death on a rainy stairwell. **Night Shift** traces missing records through an archive. **The Last Screening** reconstructs a projectionist’s movements after closing. Each case has an independent save, original glTF props and a distinct environmental scare. The [narrative design](Docs/NarrativeDesign.md) documents the stories, art and playtime targets, and PEGI 16 content target.

The C++20 engine targets Windows, Linux and macOS desktop. It ships US English, German and French in the native menus and case files. The [Godot Legacy Demo](LegacyDemo/README.md) is the earlier version of the Apartment investigation.

## Graphics engine reference

Launch the material study independently of the game. Drag with the left mouse button to orbit the scans, use the wheel to move closer and press Esc to exit.

```sh
bazelisk run //Sources/Rendering:reference -- --fullscreen
```

The desktop renderer uploads indexed meshes once per scene. OpenGL 4.2 Core, Vulkan and Metal 3 use the same GLSL sources for normal-mapped metallic/roughness shading, local reflections, depth-dependent haze, filmic tone mapping and a contrast-adaptive edge filter. Static triangle geometry supplies area-light visibility and hemispherical contact occlusion. Interior and street probes capture the scene at load time. Light intensity and camera-dependent highlights update every frame. The street's rain film lowers asphalt roughness.

On macOS 13 or later, the Objective-C++ bridge checks the default GPU for the Metal 3 family. Xcode's Metal compiler builds the shared shader stages into a native Metal 3 library. Open the study with `bazelisk run //Sources/Rendering:reference -- --graphics=metal`. Regenerating shader sources requires `glslc` and `spirv-cross`. Normal builds use the checked-in shader sources.

The reference scene includes a vaulted alcove, turned plinths, a marble bust and a brass vase. The production executable and material study share the rendering library. Jolt handles player movement and collision in the game. Asset provenance is recorded with the [materials](Assets/Materials/README.md) and [reference models](Assets/Reference/README.md).

The Apartment security monitor is an original Blender GLB. The renderer loads its embedded PBR maps at scene load and keeps its screen visible on Low. See the [case geometry](Assets/Geometry/README.md) for the supported Blender export settings.

## Run the native build

Use Bazelisk and Clang from the repository root. GCC and clang-cl are compatibility configurations.

```sh
bazelisk run //Sources/Game:23.17
bazelisk test //Tests/...
bazelisk run //Benches/Investigation:case_benchmark
```

Use **WASD** or the arrow keys to move, the **mouse** to look and **E** to interact. **J** opens the journal, where you can reread collected sources, choose records to compare, challenge testimony and order events. **R** opens the report. Case evidence, timeline edits and report drafts save automatically when you close a panel. The earlier native Apartment save is copied into the new case file when first opened. The pause menu lets you return to the case selector. In-game panels support mouse selection, arrow keys, Enter, numbered shortcuts and Esc. **Esc** opens the pause menu and **F1** goes directly to Settings. **Automatic** tries a hardware Vulkan device first and falls back to Metal 3 on macOS or OpenGL 4.2 Core on Windows and Linux. You can select a specific API from the settings and the choice is saved separately from the legacy demo. Use `-- --graphics=opengl` or `-- --graphics=vulkan` after the Bazel target to override the startup preference for one run.

From the start or pause menu, open **Settings** to change render resolution, borderless fullscreen, VSync, graphics quality, mouse sensitivity, vertical look, language, ambience volume, sudden-effect volume and sound captions. **Apply changes** saves the selected options. Borderless fullscreen uses the desktop size for presentation. The game renders at the selected resolution and fits the image to the available space.

The following command checks all three physical investigation routes without opening a window.

```sh
bazelisk run //Sources/Game:23.17 -- --verify-campaign
```

The performance route walks both directions through a case and turns the camera while rendering at High with VSync off. It logs the selected render target and the presentation window size separately.

```sh
bazelisk run //Sources/Game:23.17 -- --case=archive --benchmark-seconds=60 --1280x720
```

Use `--case=apartment`, `--case=archive` or `--case=cinema` to open a specific case. Use `--1366x768` to measure the other reference resolution.

Run `bazelisk test --config=gcc //Tests/...` for the GCC compatibility tests on Linux. On Windows, install LLVM and the Windows SDK and use `bazelisk test --config=clangcl //Tests/...`.

For a desktop archive, first run `bazelisk build //Sources/Game:23.17`. Pass the resulting executable and an output ZIP to `python3 Build/PackageDesktop.py`. Use the `.exe` build output for Windows and pass any required redistributable runtime files with `--runtime`.

Native C++ code follows the [Progmasoft Core C++ Coding Guidelines](https://github.com/Progmasoft/progmasoft-cpp). Bazel packages group implementation, headers, tests and benchmarks under `Sources`, `Headers`, `Tests` and `Benches`.

If you still have the Godot `case.json`, you can import shared observations, the matching clock comparison and the original sighting challenge into a fresh Apartment save with `python3 Build/ImportLegacySave.py GODOT_CASE_JSON NATIVE_SAVE_DIRECTORY`. The Godot badge record and the native service-exit record describe different events. The old checkpoint, timeline and report draft are specific to the Godot case.

## Continuous integration

GitHub Actions builds the native game with Clang, runs Catch3 and checks all three investigation routes. A separate CodeQL workflow traces the Bazel build. Dependabot monitors Bazel, GitHub Actions and the legacy NuGet project. Coverage from the first-party unit tests is uploaded to Codecov and to Codacy on the default branch. Codacy Cloud analyzes the repository using its C++ configuration.

Run `doxygen Doxyfile` to generate the API reference locally. Documentation warnings fail CI. The desktop prerelease workflow uses the commit-derived version and verifies the packaged archive before publication. See [Versioning](VERSIONING.md).

## Project information

- [Project website](https://23-17.leitwolf11.workers.dev/)
- [Contributing](CONTRIBUTING.md)
- [Code of conduct](CODE_OF_CONDUCT.md)
- [Security reporting](SECURITY.md)
- [Versioning](VERSIONING.md)

## Licenses

Project code uses the [MIT License](LICENSE). Game assets, including generated mesh geometry, textures, music and sound effects, use [CC0 1.0 Universal](LegacyDemo/assets/LICENSE).

The bundled [Canada 1500 typeface](https://typodermicfonts.com/canada1500/) by Raymond Larabie also uses CC0 1.0 Universal.

Copyright (c) 2026 The 23.17 Computer Game's Contributors
