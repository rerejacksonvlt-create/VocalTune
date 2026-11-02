# VocalTune

An original Windows VST3 vocal pitch-editor project.

## Current status: Stage 1 foundation

This first build establishes:
- JUCE/CMake VST3 project structure
- VocalTune plugin identity and Windows build workflow
- Parameter state save/restore
- Initial interface and parameter controls
- Transparent audio pass-through

**Important:** this is not yet a working pitch-correction processor. The controls are currently UI/parameter scaffolding. Pitch detection, note segmentation, correction DSP, waveform editing, audio/MIDI export and harmony generation are planned development stages. Do not use this build as a vocal tuning tool yet.

## Build automatically with GitHub Actions

1. Create a GitHub repository named `VocalTune`.
2. Upload the contents of this folder to the repository root (the `CMakeLists.txt`, `Source` and `.github` items must be at the root).
3. Open the repository's **Actions** tab.
4. Enable Actions if GitHub asks.
5. The workflow named **Build VocalTune VST3** runs on pushes and can also be started with **Run workflow**.
6. Open the completed workflow run and download the `VocalTune-Windows-VST3` artifact.

The workflow uses a Windows runner, Visual Studio 2022 build tools, CMake and JUCE 8.0.10. The first build downloads JUCE from its official GitHub repository, so the runner needs internet access.

## Install in FL Studio

1. Download and extract the build artifact ZIP.
2. Find `VocalTune.vst3` inside the extracted `VST3` folder.
3. Copy the complete `VocalTune.vst3` bundle into:
   `C:\\Program Files\\Common Files\\VST3`
4. Restart FL Studio.
5. Open **Options > Manage plugins** and run a scan.
6. Search for `VocalTune`.

If Windows or FL Studio blocks the plugin, check that the downloaded artifact extracted correctly and that the full `.vst3` folder—not just a file inside it—was copied.

## Development roadmap

- Stage 1: buildable plugin foundation
- Stage 2: offline audio import, waveform display and monophonic pitch analysis
- Stage 3: editable note blobs, piano-roll reference and scale mapping
- Stage 4: pitch correction DSP, retune speed, transition and drift controls
- Stage 5: time editing, formant-aware processing and rendering
- Stage 6: MIDI/audio export, harmonies, QA and performance work

## Licensing

JUCE is distributed under its own licensing terms. Review the current JUCE licence and ensure your intended distribution complies with it. VocalTune's source code and branding are a separate project.
