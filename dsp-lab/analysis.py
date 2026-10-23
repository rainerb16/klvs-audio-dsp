"""Shared measurement helpers and test signals."""
from __future__ import annotations

import numpy as np
import pyloudnorm as pyln
from scipy import signal


def lufs(x, fs) -> float:
    return pyln.Meter(fs).integrated_loudness(np.atleast_2d(x).T)


def pink_noise(n, fs, seed=1, channels=2):
    rng = np.random.default_rng(seed)
    spec = np.fft.rfft(rng.standard_normal((channels, n)), axis=-1)
    f = np.fft.rfftfreq(n, 1 / fs)
    f[0] = f[1]
    x = np.fft.irfft(spec / np.sqrt(f), n, axis=-1)
    # Band-limit to 20 Hz..20 kHz like real program material (raw 1/f noise is mostly infrasonic).
    sos = signal.butter(4, [20, min(20000, 0.45 * fs)], "bandpass", fs=fs, output="sos")
    x = signal.sosfilt(sos, x, axis=-1)
    return x / np.max(np.abs(x))


def set_lufs(x, fs, target):
    return x * 10 ** ((target - lufs(x, fs)) / 20)


def sine(freq, dbfs, fs, seconds=1.5, channels=2):
    t = np.arange(int(fs * seconds)) / fs
    return np.tile(10 ** (dbfs / 20) * np.sin(2 * np.pi * freq * t), (channels, 1))


def harmonics(y, fs, f0, n_harm=5, seconds=1.0):
    """Levels of harmonics 2..n relative to the fundamental (dB), plus THD %.
    Analyses the last `seconds` of channel 0; f0*seconds must be an integer."""
    n = int(fs * seconds)
    seg = y[0, -n:]
    spec = np.abs(np.fft.rfft(seg * signal.windows.blackmanharris(n)))
    def peak(f):
        k = int(round(f * seconds))
        return np.max(spec[max(k - 3, 0):k + 4]) if f < fs / 2 else 0.0
    fund = peak(f0)
    hs = {h: peak(h * f0) for h in range(2, n_harm + 1)}
    thd = 100 * np.sqrt(sum(v ** 2 for v in hs.values())) / fund
    return {h: 20 * np.log10(v / fund + 1e-12) for h, v in hs.items()}, thd


def band_db(x, fs, lo, hi):
    """Energy (dB) of channel-summed x within [lo, hi) Hz."""
    m = np.mean(x, axis=0)
    spec = np.abs(np.fft.rfft(m)) ** 2
    f = np.fft.rfftfreq(len(m), 1 / fs)
    return 10 * np.log10(np.sum(spec[(f >= lo) & (f < hi)]) + 1e-20)


def make_test_mix(fs, seconds=20, seed=3):
    """Stand-in 'mix' (pink noise bed + kick + sub bass) for checks without a real file.
    Not for listening judgements."""
    n = int(fs * seconds)
    t = np.arange(n) / fs
    bed = pink_noise(n, fs, seed) * 0.25
    kick = np.zeros(n)
    klen = int(0.35 * fs)
    kt = np.arange(klen) / fs
    kfreq = 50 + 90 * np.exp(-kt * 30)
    one = np.sin(2 * np.pi * np.cumsum(kfreq) / fs) * np.exp(-kt * 9)
    for start in range(0, n - klen, int(0.5 * fs)):
        kick[start:start + klen] += one
    bass = 0.3 * np.sin(2 * np.pi * 41.2 * t) * (0.6 + 0.4 * np.sin(2 * np.pi * 0.25 * t))
    x = bed + 0.7 * kick + bass
    x = x / np.max(np.abs(x)) * 10 ** (-3 / 20)  # peaks at -3 dBFS
    return x
