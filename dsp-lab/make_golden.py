"""Write golden test vectors for the C++ engine test (tests/golden/).

    python make_golden.py

input.wav is processed by the Python reference (wrm1.py) at each case's settings; the C++ test
runs the same input through the plugin engine and compares. Re-run whenever wrm1.py changes.
"""
import json
from pathlib import Path

import numpy as np
import soundfile as sf
from scipy import signal

import wrm1
from analysis import pink_noise
from wrm1 import Params

FS = 48000
OUT = Path(__file__).resolve().parent.parent / "tests/golden"
OS_INDEX = {1: 0, 2: 1, 4: 2}  # Python factor -> plugin choice index

CASES = [
    ("defaults_tube", Params(oversampling=1)),
    ("tape_dark_full_protect", Params(character="tape", warmth=0.7, tone=-0.6, body=0.8, low_protect=1.0, oversampling=1)),
    ("console_hot_bright", Params(character="console", warmth=1.0, input_db=6.0, tone=0.8, body=0.0, low_protect=0.0, oversampling=1)),
    ("tube_parallel_no_autogain", Params(warmth=0.6, mix=0.5, output_db=-3.0, auto_gain=False, oversampling=1)),
    ("tape_2x_parallel", Params(character="tape", warmth=0.5, mix=0.6, oversampling=2)),
    ("tube_4x_heavy", Params(warmth=0.9, oversampling=4)),
    ("tube_4x_mild", Params(warmth=0.5, oversampling=4)),
    ("tube_2x_heavy", Params(warmth=0.9, oversampling=2)),
    ("tube_1x_heavy", Params(warmth=0.9, oversampling=1)),
    ("tube_4x_parallel", Params(warmth=0.6, mix=0.5, oversampling=4)),
]


def make_input():
    n = FS // 2
    t = np.arange(n) / FS
    noise = pink_noise(n, FS, seed=5)
    # keep the noise below 16 kHz so oversampled cases compare program material, not filter edges
    noise = signal.sosfilt(signal.butter(8, 16000, fs=FS, output="sos"), noise, axis=-1)
    tones = np.vstack([0.25 * np.sin(2 * np.pi * 60 * t) + 0.1 * np.sin(2 * np.pi * 1000 * t),
                       0.2 * np.sin(2 * np.pi * 55 * t) + 0.1 * np.sin(2 * np.pi * 1500 * t)])
    x = 0.35 * noise / np.max(np.abs(noise)) + tones
    return (x / np.max(np.abs(x)) * 10 ** (-3 / 20)).astype(np.float32)


def write_if_changed(path, audio):
    """Write only if the samples differ: the WAV writer stamps the time into the header, so
    rewriting identical audio would show every golden file as changed in git."""
    if path.exists():
        old, _ = sf.read(path, dtype="float32", always_2d=True)
        if old.shape == audio.T.shape and np.array_equal(old, audio.T):
            return
    sf.write(path, audio.T, FS, subtype="FLOAT")


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    x = make_input()
    write_if_changed(OUT / "input.wav", x)
    manifest = {"sampleRate": FS, "input": "input.wav", "cases": []}
    for name, p in CASES:
        y = wrm1.process(x.astype(np.float64), FS, p).astype(np.float32)
        write_if_changed(OUT / f"{name}.wav", y)
        manifest["cases"].append({
            "name": name, "file": f"{name}.wav",
            "settings": {"input": p.input_db, "warmth": p.warmth, "tone": p.tone, "body": p.body,
                         "character": list(wrm1.CHARACTERS).index(p.character), "lowProtect": p.low_protect,
                         "mix": p.mix, "autoGain": p.auto_gain, "output": p.output_db,
                         "oversampling": OS_INDEX[p.oversampling]},
            # Off: same maths, float vs double. 2x/4x: different (equally valid) anti-alias filters.
            "maxResidualDb": -90.0 if p.oversampling == 1 else -50.0,
        })
        print(f"{name:28s} written")
    (OUT / "manifest.json").write_text(json.dumps(manifest, indent=1) + "\n")


if __name__ == "__main__":
    main()
