# Rainr Audio conventions

Rules every Rainr Audio plugin follows. They exist so that saved projects keep working and
so that the next plugin is quick to start.

## Repository layout

```
JUCE/                 JUCE framework, git submodule pinned to a release tag
cmake/RainrPlugin.cmake   rainr_add_plugin(): company-wide plugin settings
shared/rainr_dsp/     DSP shared by all plugins (JUCE module)
shared/rainr_ui/      Brand look: colours, fonts, knobs, meters, scope (JUCE module)
                      assets/fonts: IBM Plex, SIL Open Font License (OFL.txt ships with it)
plugins/<CODE>/       One folder per plugin: Source/, VERSION, CMakeLists.txt
tests/                C++ engine tests against the Python reference (golden files)
tools/                Development helpers, e.g. WRM1Snapshot (renders the UI to a PNG)
dsp-lab/              Python reference models and listening tools
docs/                 Specs (<CODE>_DSP.md) and this file
CHANGELOG.md          One section per plugin
```

Adding a plugin: create `plugins/<CODE>/`, call `rainr_add_plugin()` from its
`CMakeLists.txt`, add `add_subdirectory(plugins/<CODE>)` to the top-level `CMakeLists.txt`, and
register its codes below. Product images (logo lockups) go in `plugins/<CODE>/Assets/` and are
passed as `ASSETS`; they are embedded and readable as `<CODE>Assets::<file>_png`.

## Permanent identifiers

Hosts find a plugin in a saved project by these values. Changing any of them after a build has
been shared breaks every project that uses the plugin.

| What | Value | Where |
|------|-------|-------|
| Company name | `Rainr Audio` | `cmake/RainrPlugin.cmake` |
| Manufacturer code | `Rnra` | `cmake/RainrPlugin.cmake` |

| Plugin | Plugin code | Bundle ID | Status |
|--------|-------------|-----------|--------|
| WRM1 | `Wrm1` | `com.rainraudio.wrm1` | in development |
| SPM1 | `Spm1` | `com.rainraudio.spm1` | reserved |

Plugin codes are four characters with exactly one upper-case letter.

## Parameters

- IDs are lowerCamelCase English words: `warmth`, `lowProtect`, `autoGain`.
- IDs are permanent: never rename, re-range or remove. Choice parameters are stored as an index,
  so choices are only ever appended.
- Every parameter carries a version hint. Everything in a plugin's first release uses `1`;
  parameters added later use the version number of the release that adds them.
- All IDs for a plugin live in one file, `plugins/<CODE>/Source/Parameters.h`, under the
  permanent-ID banner.
- Parameters that change latency (oversampling) are not automatable.
- Bypass is the host's bypass, not a plugin parameter.

## Saved state

State is the parameter tree saved as XML with two extra properties: `stateFormatVersion`
(bumped only when the format itself changes, so old projects can be migrated) and
`pluginVersion`. Loading ignores unknown parameters and keeps defaults for missing ones.

## Versions

- Each plugin has its own semantic version in `plugins/<CODE>/VERSION` (`0.1.0`). It is
  compiled in and shown in the plugin window.
  - patch (0.1.**1**): fixes, no sound change on existing settings
  - minor (0.**2**.0): new features or sound changes that keep projects loading
  - major (**1**.0.0): first public release, and later anything that breaks compatibility
- Release tags name the plugin: `wrm1-v0.1.0`.
- Every version gets an entry in `CHANGELOG.md` under that plugin's heading, newest first,
  written for musicians (what changed in the sound or behaviour), not for programmers.

## Releasing a test build

1. Set the version in `plugins/<CODE>/VERSION` and move the changelog entry from
   "Unreleased" to that version with today's date. Commit and push; wait for a green build.
2. Tag and push: `git tag wrm1-v0.1.0 && git push origin wrm1-v0.1.0`.
3. CI builds again, checks the tag matches `VERSION`, and publishes both zips as a pre-release on
   the Releases page (downloadable without a GitHub account). Each zip contains a short
   `README.txt`; the full guide is `docs/INSTALL.md`.

Builds are unsigned until signing is set up, so releases stay marked as pre-releases.

## JUCE version

JUCE is pinned to a release tag as a git submodule. Upgrading is a deliberate change: update the
submodule, rebuild, run pluginval and the DSP tests, and note it in the changelog.

## Commits

Plain, descriptive messages in the imperative ("Add Low Protect smoothing"). No tool or AI
attribution lines.
