"""Render a mix through WRM1 at several settings, loudness-matched to the dry file.

    python render.py path/to/mix.wav            # writes renders/<mix name>/*.wav
    python render.py path/to/mix.wav --blind    # also writes a shuffled copy + answer key
    python render.py path/to/mix.wav --start 120 --length 60   # excerpt (seconds)

Outputs are 32-bit float WAV (cannot clip) and matched to the dry file's integrated
LUFS, so you compare tone, not level. The table printed at the end is also saved
as INDEX.txt next to the renders.
"""
import argparse
import random
import shutil
from pathlib import Path

import numpy as np
import soundfile as sf

import wrm1
from analysis import band_db, lufs
from wrm1 import Params

D = Params()  # defaults: tube, warmth 0.4, body 0.3, LP 0.5, 2x OS

PRESETS = [
    ("tube_w40", D.with_(character="tube", warmth=0.4)),
    ("tube_w60", D.with_(character="tube", warmth=0.6)),
    ("tube_w80", D.with_(character="tube", warmth=0.8)),
    ("tube_w100", D.with_(character="tube", warmth=1.0)),
    ("tape_w40", D.with_(character="tape", warmth=0.4)),
    ("tape_w60", D.with_(character="tape", warmth=0.6)),
    ("tape_w80", D.with_(character="tape", warmth=0.8)),
    ("console_w40", D.with_(character="console", warmth=0.4)),
    ("console_w60", D.with_(character="console", warmth=0.6)),
    ("console_w80", D.with_(character="console", warmth=0.8)),
    ("tube_w80_lowprotect-0", D.with_(warmth=0.8, low_protect=0.0)),
    ("tube_w80_lowprotect-100", D.with_(warmth=0.8, low_protect=1.0)),
    ("tube_w60_tone-dark50", D.with_(warmth=0.6, tone=-0.5)),
    ("tube_w60_tone-bright50", D.with_(warmth=0.6, tone=0.5)),
    ("tube_w60_body-0", D.with_(warmth=0.6, body=0.0)),
    ("tube_w60_body-100", D.with_(warmth=0.6, body=1.0)),
    ("tube_w60_input+6", D.with_(warmth=0.6, input_db=6.0)),
    ("tube_w80_os-1x", D.with_(warmth=0.8, oversampling=1)),
]

# "What it adds": distortion only (processed minus the same chain with a linear curve),
# at its true level. Solo these to learn what the saturation sounds like.
ADDS = [("adds_tube_w60", D.with_(warmth=0.6)), ("adds_tape_w80", D.with_(character="tape", warmth=0.8)),
        ("adds_tube_w100", D.with_(warmth=1.0))]


def load(path):
    x, fs = sf.read(path, always_2d=True, dtype="float64")
    x = x.T
    if x.shape[0] == 1:
        x = np.vstack([x, x])
    return x[:2], fs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("mix")
    ap.add_argument("--out", default="renders")
    ap.add_argument("--blind", action="store_true")
    ap.add_argument("--start", type=float, default=0.0, help="excerpt start (s)")
    ap.add_argument("--length", type=float, default=None, help="excerpt length (s); default: whole file")
    args = ap.parse_args()

    x, fs = load(args.mix)
    s0 = int(args.start * fs)
    x = x[:, s0: s0 + int(args.length * fs) if args.length else None]
    out = Path(args.out) / Path(args.mix).stem
    out.mkdir(parents=True, exist_ok=True)
    dry_l = lufs(x, fs)
    sf.write(out / "00_dry.wav", x.T, fs, subtype="FLOAT")

    lines = [f"source: {args.mix}  {fs} Hz  {dry_l:.1f} LUFS  peak {20*np.log10(np.max(np.abs(x))):.1f} dBFS",
             "all files loudness-matched to 00_dry.wav",
             "",
             f"{'file':34s} {'AG off':>8s} {'AG err':>7s} {'match':>7s} {'<100Hz':>7s} {'>8kHz':>7s} {'peak':>7s}"]
    for i, (name, p) in enumerate(PRESETS, start=1):
        y = wrm1.process(x, fs, p)
        y_off = wrm1.process(x, fs, p.with_(auto_gain=False))
        ag_off = lufs(y_off, fs) - dry_l          # level change before auto gain
        ag_err = lufs(y, fs) - dry_l              # what Auto Gain left over
        y = y * 10 ** (-ag_err / 20)              # exact match for listening
        fname = f"{i:02d}_{name}.wav"
        sf.write(out / fname, y.T, fs, subtype="FLOAT")
        lo = band_db(y, fs, 20, 100) - band_db(x, fs, 20, 100)
        hi = band_db(y, fs, 8000, 20000) - band_db(x, fs, 8000, 20000)
        pk = 20 * np.log10(np.max(np.abs(y)))
        lines.append(f"{fname:34s} {ag_off:+7.2f}  {ag_err:+6.2f}  {-ag_err:+6.2f}  {lo:+6.2f}  {hi:+6.2f}  {pk:+6.1f}")
        print(lines[-1], flush=True)
    adds = out / "what-it-adds"
    adds.mkdir(exist_ok=True)
    for name, p in ADDS:
        r = wrm1.process(x, fs, p) - wrm1.process(x, fs, p.with_(curve_bypass=True))
        sf.write(adds / f"{name}.wav", r.T, fs, subtype="FLOAT")
        lines.append(f"what-it-adds/{name}.wav: {20 * np.log10(np.std(r) / np.std(x)):+.1f} dB rel. mix")
    lines += ["",
              "AG off = LUFS change with Auto Gain off (level change before auto gain)",
              "AG err = how far Auto Gain missed (0 = perfect); match = extra gain applied for this file",
              "<100Hz / >8kHz = band energy change vs dry after matching (dB); peak in dBFS"]
    (out / "INDEX.txt").write_text("\n".join(lines) + "\n")

    if args.blind:
        bdir = out / "blind"
        bdir.mkdir(exist_ok=True)
        files = sorted(f for f in out.glob("*.wav"))
        random.shuffle(files)
        key = []
        for j, f in enumerate(files, start=1):
            shutil.copy(f, bdir / f"{chr(64 + j) if j <= 26 else j}.wav")
            key.append(f"{chr(64 + j) if j <= 26 else j}: {f.name}")
        (out / "blind_KEY.txt").write_text("\n".join(key) + "\n")
        print(f"blind set in {bdir}, key in blind_KEY.txt (don't peek)")
    print(f"\nwrote {out}")


if __name__ == "__main__":
    main()
