# Rainr Audio plugins

Audio plugins by Rainr Audio, built with [JUCE](https://juce.com).

| Plugin | What it does | Status |
|--------|--------------|--------|
| WRM1 | Mix-bus warmth saturator | in development |

## Building on a Mac

Requires Xcode (command-line use is enough) and CMake 3.22+.

```
git clone --recurse-submodules https://github.com/rainerb16/rainr-audio-dsp.git
cd rainr-audio-dsp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target WRM1_VST3 WRM1_AU -j8
```

Plugins land in `build/plugins/WRM1/WRM1_artefacts/Release/`. Local builds are for the Mac you
are on; add `-DRAINR_UNIVERSAL=ON` for Intel + Apple Silicon.

## Docs

- [docs/WRM1_DSP.md](docs/WRM1_DSP.md): the WRM1 sound, specified
- [docs/CONVENTIONS.md](docs/CONVENTIONS.md): IDs, versions, repo layout
- [dsp-lab/README.md](dsp-lab/README.md): Python reference model and listening tools
- [CHANGELOG.md](CHANGELOG.md)
