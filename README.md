# ABYSS — Dark Atmospheric Vocal Processor

A JUCE/CMake VST3 starter project for Windows. ABYSS uses one front-loaded macro control to blend warm saturation, darkening, chorus, stereo flanger, stereo phaser and atmospheric reverb. Expert Mode exposes the underlying effect parameters.

## Build on Windows

1. Install Visual Studio 2022 with **Desktop development with C++**.
2. Install CMake 3.22+ and Git.
3. Clone this repository.
4. Run:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
```

The VST3 bundle is normally created under:

`build\AbyssVocal_artefacts\Release\VST3\ABYSS.vst3`

Copy the complete `.vst3` bundle to:

`C:\Program Files\Common Files\VST3`

Restart/rescan plugins in your DAW.

## GitHub Actions

Push the repository to GitHub. The workflow in `.github/workflows/build-windows.yml` builds on Windows and publishes a downloadable ZIP as a workflow artifact. You can also run it manually from **Actions → Build Windows VST3 → Run workflow**.

## Sound design notes

The macro uses a front-loaded power curve (`amount ^ 0.48`) so the first part of the control changes the tone quickly. The later range increases modulation and space. Expert Mode provides independent trims for Warmth, Dark, Chorus, Detune, Wobble, Flanger, Phaser, Atmosphere, Width and Wet.

## Important prototype notes

- This is a functional starting point, not a finished commercial release.
- The display is a custom animated signal-inspired visualization; it is not an OLED hardware display or a spectrum analyzer.
- The DSP is a lightweight first pass. Before commercial use, add oversampling/anti-aliasing for saturation, proper stereo-linked modulation, parameter smoothing, latency/tail validation, preset management, accessibility and extensive DAW testing.
- The installer script is a template and may need its source path adjusted to the exact JUCE build output.
- JUCE is fetched from its official GitHub repository at configure time. Review JUCE's current licensing terms for your distribution model.


### Compatibility fix (JUCE 8)

The reverb is initialized with `juce::dsp::Reverb::prepare()` and processed using `juce::dsp::ProcessContextReplacing`, matching the JUCE 8 DSP API.
