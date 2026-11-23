# 23.17 — An Apartment Case

23.17 is a first-person, single-player 3D detective game set in Istanbul. On a rainy November night, a resident is found dead in a stairwell. A witness claims to have seen the resident at 23:17 on a floor where the power was out.

Explore the apartment building, examine evidence and question witnesses. Cross-check clocks and statements in your notebook, reconstruct the timeline and use the evidence to support your final report.

## Development status

The game is in planning. The first playable prototype will cover the entrance and fourth-floor landing, with one witness, three pieces of evidence and a report decision.

| Area | Target |
| --- | --- |
| Engine | Godot 4 .NET, C# and the Compatibility renderer |
| Platforms | Windows and Linux desktop, with keyboard and mouse |
| Resolutions | 960×540, 1280×720, 1920×1080 and 2560×1440 |
| Graphics | Low, Medium, High, Xhigh, Max and Custom. First-launch defaults depend on hardware. |
| Text languages | US English (`en-US`), German (`de-DE`) and French (`fr-FR`) |

The prototype's performance target is at least 50 FPS at 1280×720 on **High** on the reference Linux PC with an Intel Core i5-3210M and Intel HD Graphics 4000. During traversal, evidence inspection and dialogue, at least 99% of measured frames should take 20 ms or less.

## Project information

- [Project website](https://23-17.leitwolf11.workers.dev/)
- [Contributing](CONTRIBUTING.md)
- [Code of conduct](CODE_OF_CONDUCT.md)
- [Security reporting](SECURITY.md)
- [Versioning](VERSIONING.md)

## Licenses

Project code uses the [MIT License](LICENSE). Game assets, including models, textures, music and sound effects, use [CC0 1.0 Universal](assets/LICENSE).

Copyright (c) 2026 The 23.17 Computer Game's Contributors
