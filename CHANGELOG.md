# Changelog

One section per plugin, newest version first. Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## WRM1

### [Unreleased] 0.1.0
- Project set up: plugin builds as VST3 and AU, with all v1 parameters (Input, Warmth, Tone,
  Body, Character, Low Protect, Mix, Auto Gain, Output, Oversampling).
- Saturation engine: the full WRM1 sound from the Phase 1 design (Tape, Tube, Console curves,
  Low Protect, Tone, Body, HF rolloff, Auto Gain, Mix, Output) with smooth control changes and
  2x/4x oversampling. Matches the Python reference; light on CPU (about 1.3 % of one core at 2x).
- Plugin window: the Rainr Audio hardware look with the WRM1 lockup, large Warmth knob, Character
  and Oversampling switches, input/output meters, Auto Gain switch and a scope showing the
  saturation curve, harmonics and the Auto Gain amount. Double-click any knob to reset it.
