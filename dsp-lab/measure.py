"""Print WRM1 measurements: harmonics, level change before auto gain, Low Protect, aliasing.

    python measure.py                 # uses a synthetic stand-in mix
    python measure.py path/to/mix.wav # uses your mix for the mix-based numbers
"""
import sys

import numpy as np
import soundfile as sf

import wrm1
from analysis import band_db, harmonics, lufs, make_test_mix, sine
from wrm1 import Params

FS = 48000
BASE = Params(body=0.0, tone=0.0, low_protect=0.0, oversampling=4)


def sanity():
    x = np.random.default_rng(0).standard_normal((2, FS))
    b1 = wrm1.low_shelf(FS, 100, -12); b2 = wrm1.low_shelf(FS, 100, 12)
    err = np.max(np.abs(wrm1.apply(b2, wrm1.apply(b1, x)) - x))
    u = np.array([-1e-6, 1e-6])
    slopes = {c: float(np.diff(wrm1.shaper(u, c))[0] / 2e-6) for c in wrm1.CHARACTERS}
    print(f"sanity: shelf/inverse max error {err:.1e}; small-signal slope {slopes}")


def harmonic_table():
    print("\n== Harmonics of a 1 kHz sine (dB rel. fundamental; 4x OS; no EQ moves) ==")
    for level in (-18, -6):
        print(f"\n input {level} dBFS   warmth  drive   H2     H3     H4     H5    THD%   even/odd")
        for ch in wrm1.CHARACTERS:
            for w in (0.2, 0.4, 0.6, 0.8, 1.0):
                y = wrm1.process(sine(1000, level, FS), FS, BASE.with_(character=ch, warmth=w))
                h, thd = harmonics(y, FS, 1000)
                eo = 10 * np.log10((10 ** (h[2] / 10) + 10 ** (h[4] / 10)) /
                                   (10 ** (h[3] / 10) + 10 ** (h[5] / 10)))
                print(f"  {ch:8s}  {w:5.2f}  {wrm1.drive_db(w):+5.1f}  "
                      + "  ".join(f"{h[k]:6.1f}" for k in (2, 3, 4, 5))
                      + f"  {thd:6.3f}  {eo:+5.1f} dB")


def mix_table(x, fs):
    print("\n== On the mix: level change before auto gain, auto gain accuracy ==")
    print("  (defaults: LP 0.5, body 0.3, 2x OS)")
    print("  character warmth  LUFS change(AG off)  AG applied  residual after AG  peak change")
    dry_l = lufs(x, fs)
    dry_pk = 20 * np.log10(np.max(np.abs(x)))
    for ch in wrm1.CHARACTERS:
        for w in (0.2, 0.4, 0.6, 0.8, 1.0):
            p = Params(character=ch, warmth=w)
            off = wrm1.process(x, fs, p.with_(auto_gain=False))
            ag = wrm1.autogain_db(p)
            d = lufs(off, fs) - dry_l
            pk = 20 * np.log10(np.max(np.abs(off))) - dry_pk
            print(f"  {ch:8s}  {w:5.2f}   {d:+6.2f} LU            {ag:+5.2f} dB   {d + ag:+5.2f} LU          {pk:+5.2f} dB")


def low_protect_table(x, fs):
    print("\n== Low Protect (tube, warmth 0.6, 2x OS) ==")
    for lp in (0.0, 1.0):
        y = wrm1.process(sine(50, -10, fs, seconds=2.0), fs,
                         BASE.with_(character="tube", warmth=0.6, low_protect=lp))
        h, thd = harmonics(y, fs, 50)
        print(f"  50 Hz sine @ -10 dBFS, LP {lp:.0%}:  H2 {h[2]:6.1f}  H3 {h[3]:6.1f} dB   THD {thd:.3f}%")
    print("  on mix, after loudness matching, change vs dry in each band:")
    dry_l = lufs(x, fs)
    for lp in (0.0, 0.5, 1.0):
        y = wrm1.process(x, fs, Params(character="tube", warmth=0.6, low_protect=lp, body=0.0))
        y = y * 10 ** ((dry_l - lufs(y, fs)) / 20)
        bands = [(20, 60), (60, 100), (100, 300), (2000, 8000), (8000, 20000)]
        s = "  ".join(f"{lo}-{hi}Hz {band_db(y, fs, lo, hi) - band_db(x, fs, lo, hi):+5.2f}" for lo, hi in bands)
        print(f"    LP {lp:4.0%}: {s}")


def aliasing_table():
    print("\n== Aliasing: 7 kHz sine @ -6 dBFS, tube, warmth 0.8 (non-harmonic energy rel. fundamental) ==")
    f0 = 7000
    for os_ in (1, 2, 4):
        y = wrm1.process(sine(f0, -6, FS), FS, BASE.with_(character="tube", warmth=0.8, oversampling=os_))
        seg = y[0, -FS:]
        spec = np.abs(np.fft.rfft(seg * np.blackman(FS))) ** 2
        f = np.arange(len(spec))
        harm = np.zeros_like(spec, bool)
        for k in range(1, 4):
            harm[max(k * f0 - 5, 0):k * f0 + 6] = True
        alias = 10 * np.log10(spec[~harm & (f > 20)].sum() / spec[f0 - 5:f0 + 6].sum())
        print(f"  {os_}x: {alias:6.1f} dB")


def main():
    sanity()
    if len(sys.argv) > 1:
        x, fs = sf.read(sys.argv[1], always_2d=True)
        x = x.T if x.shape[1] > 1 else np.vstack([x.T, x.T])
        x = x[:2, : int(fs * 60)]  # first minute is plenty for numbers
        print(f"mix: {sys.argv[1]}  ({fs} Hz, {lufs(x, fs):.1f} LUFS)")
    else:
        fs = FS
        x = make_test_mix(fs)
        print(f"mix: synthetic stand-in ({lufs(x, fs):.1f} LUFS)")
    harmonic_table()
    mix_table(x, fs)
    low_protect_table(x, fs)
    aliasing_table()


if __name__ == "__main__":
    main()
