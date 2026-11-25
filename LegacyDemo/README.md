# 23.17 Legacy Demo

The Godot .NET prototype of the first apartment case.

## Development status

The playable case spans the entrance, basement service room and third- and fourth-floor landings. Examine four physical records, interview the caretaker and the electrician, and compare their accounts with the building records. Reconstruct six events in the report to reach one of three outcomes. Original textures, animated witnesses, spatial ambience, footsteps and music support the investigation.

| Area | Target |
| --- | --- |
| Engine | Godot 4 .NET, C# and the Compatibility renderer |
| Platforms | Windows and Linux desktop, with keyboard and mouse |
| Resolutions | 960×540, 1280×720, 1366×768, 1920×1080 and 2560×1440 |
| Graphics | Low, Medium, High, Xhigh, Max and Custom. First-launch defaults depend on hardware. |
| Text languages | US English (`en-US`), German (`de-DE`) and French (`fr-FR`) |

The prototype's performance target is at least 50 FPS at 1280×720 on **High** on the reference Linux PC with an Intel Core i5-3210M and Intel HD Graphics 4000. During traversal, evidence inspection and dialogue, at least 99% of measured frames should take 20 ms or less.

## Run the prototype

Open `project.godot` in Godot 4.7.2 .NET with a .NET 8 SDK or newer installed, then press **F6** from the main scene or **F5** from the project. From the command line, run `godot --headless --path . --import` on a fresh checkout, then run `dotnet build` followed by `godot --path .` from the LegacyDemo directory.

Use **WASD** or the arrow keys to walk, the **mouse** to look, **E** to interact, **J** to open the notebook, **R** to file a report and **Esc** to access settings. For the reference graphics configuration, launch with `godot --path . -- --reference-high`.

Use **Compare sources** in the notebook to link collected records and statements. The notebook keeps each source’s original timestamps and shows corrections alongside the evidence. Use the up and down buttons in the report to order discovered events from earliest to latest. Adjust ambience, music and effects independently in Settings.

The game automatically saves collected evidence, interviews, comparisons, timeline order and report answers. **Continue investigation** resumes at the last safe landing. **Restart investigation** asks for confirmation before clearing case progress. Graphics, language and audio settings are saved separately.
