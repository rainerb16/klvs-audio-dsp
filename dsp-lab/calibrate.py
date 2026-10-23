"""Build autogain_table.json: the static loudness compensation used by Auto Gain.

Reference: your mixes (preferred) or band-limited pink noise, normalised to -16 LUFS
(typical pre-master mix level). Run again whenever curves or constants change:
    python calibrate.py mix1.wav mix2.wav ...   # calibrate on real music
    python calibrate.py                         # fallback: pink noise
"""
import json
import pathlib
import sys

import numpy as np
import soundfile as sf
from scipy import signal

import wrm1
from analysis import lufs, pink_noise, set_lufs

FS = 48000
REF_LUFS = -16.0
# Drive grid covers Warmth 0..1 plus Input trim +-12 dB.
DRIVE_GRID = list(range(int(wrm1.DRIVE_MIN_DB - wrm1.INPUT_RANGE_DB),
                        int(wrm1.DRIVE_MAX_DB + wrm1.INPUT_RANGE_DB) + 1, 3))
LP_GRID = [0.0, 0.5, 1.0]


def gain_needed(x, p):
    y = wrm1.process(x, FS, p.with_(auto_gain=False, mix=1.0))
    return REF_LUFS - lufs(y, FS)


def main():
    if wrm1.AUTOGAIN_TABLE_PATH.exists():
        wrm1.AUTOGAIN_TABLE_PATH.unlink()
        wrm1._table_cache = None
    if len(sys.argv) > 1:
        parts = []
        for path in sys.argv[1:]:
            y, fs = sf.read(path, always_2d=True, dtype="float64")
            y = y.T if y.shape[1] > 1 else np.vstack([y.T, y.T])
            if fs != FS:
                y = signal.resample_poly(y, FS, fs, axis=-1)
            # loudest 20 s of each song (where saturation matters and loudness is judged)
            e = np.convolve(np.mean(y[:2] ** 2, axis=0)[::FS // 10], np.ones(200), "valid")
            s0 = int(np.argmax(e)) * (FS // 10)
            parts.append(set_lufs(y[:2, s0: s0 + 20 * FS], FS, REF_LUFS))
        x = np.concatenate(parts, axis=-1)
        source = ", ".join(pathlib.Path(a).name for a in sys.argv[1:])
    else:
        x = set_lufs(pink_noise(FS * 15, FS), FS, REF_LUFS)
        source = "band-limited pink noise"
    base = wrm1.Params(body=0.0, tone=0.0)
    table = {"reference": f"{source} @ {REF_LUFS} LUFS, {FS} Hz",
             "drive_grid_db": DRIVE_GRID, "low_protect_grid": LP_GRID, "characters": {}}
    for ch in wrm1.CHARACTERS:
        p = base.with_(character=ch)
        # warmth=0 + input trim sweeps drive across the grid exactly
        rows = [[round(gain_needed(x, p.with_(warmth=0.0, input_db=d - wrm1.DRIVE_MIN_DB, low_protect=lp)), 3)
                 for d in DRIVE_GRID] for lp in LP_GRID]
        quiet = p.with_(warmth=0.0, low_protect=0.0)
        g0 = gain_needed(x, quiet)
        table["characters"][ch] = {
            "drive_comp_db": rows,
            "body_db_per_unit": round(gain_needed(x, quiet.with_(body=1.0)) - g0, 3),
            "tone_db_pos": round(gain_needed(x, quiet.with_(tone=1.0)) - g0, 3),
            "tone_db_neg": round(gain_needed(x, quiet.with_(tone=-1.0)) - g0, 3),
        }
        print(ch, "LP=0.5:", [float(v) for v in rows[1]])
    wrm1.AUTOGAIN_TABLE_PATH.write_text(json.dumps(table, indent=1))
    print("wrote", wrm1.AUTOGAIN_TABLE_PATH)


if __name__ == "__main__":
    main()
