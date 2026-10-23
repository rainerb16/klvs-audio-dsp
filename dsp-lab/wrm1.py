"""WRM1 reference DSP (offline, Python).

This file is the source of truth for the sound. Every block maps 1:1 onto a C++
class in Phase 2, and the constants below are what docs/WRM1_DSP.md will freeze.

Signal flow (wet path):
    in -> Input trim -> Low Protect pre-shelf -> [Tape HF pre-emphasis]
       -> drive -> Character curve -> /drive   (oversampled)
       -> DC blocker -> [Tape HF de-emphasis] -> Low Protect inverse shelf
       -> HF rolloff -> Tone tilt -> Body bell -> Auto Gain
    out = dry*(1-mix) + wet*mix, then Output trim.

Audio is float64, shape (channels, samples). Parameters are static per render
(the plugin smooths them; offline we don't need to).
"""
from __future__ import annotations

import json
from dataclasses import dataclass, replace
from pathlib import Path

import numpy as np
from scipy import signal

# ----------------------------------------------------------------------------
# Tuning constants: this block is the spec
# ----------------------------------------------------------------------------
DRIVE_MIN_DB = -4.0       # drive at Warmth 0 (near-clean on mix-level material)
DRIVE_MAX_DB = 14.0       # drive at Warmth 1 (heavy, not clipping)
WARMTH_TAPER = 1.0        # linear in dB; measured on real mixes: 40% ~0.5 dB, 60% ~1.5-2 dB,
                          # 80% ~2-3.5 dB, 100% ~3-6 dB of peak reduction
INPUT_RANGE_DB = 12.0     # Input trim +-12 dB; adds to drive, Auto Gain compensates it

LOW_PROTECT_FREQ = 100.0  # Hz, low-shelf corner for pre/de-emphasis
LOW_PROTECT_MAX_DB = 12.0 # how far lows are pulled down before the curve at 100%

HF_ROLLOFF_FREQ = 7000.0  # Hz, high-shelf corner
HF_ROLLOFF_DB = 2.0       # shelf cut at Warmth 1 (plus per-character extra)

TONE_LOW_FREQ = 500.0     # tilt: low shelf here...
TONE_HIGH_FREQ = 2000.0   # ...and high shelf here
TONE_MAX_DB = 2.0         # each shelf moves +-2 dB (4 dB total tilt)

BODY_FREQ = 180.0         # Hz, low-mid bell
BODY_Q = 0.8
BODY_MAX_DB = 3.0

DC_BLOCK_FREQ = 10.0      # Hz, 1-pole highpass after the asymmetric curve

CHARACTERS = {
    # curve: shape family; bias: asymmetry (-> 2nd harmonic);
    # hf_emph_db: pre/de-emphasis so highs saturate more (tape HF compression);
    # hf_extra_db: extra HF shelf cut at Warmth 1 on top of HF_ROLLOFF_DB.
    "tape":    dict(curve="tanh",  bias=0.15, hf_emph_db=4.0, hf_emph_freq=3000.0, hf_extra_db=1.5),
    "tube":    dict(curve="tanh",  bias=0.25, hf_emph_db=0.0, hf_emph_freq=3000.0, hf_extra_db=0.5),
    "console": dict(curve="cubic", bias=0.06, hf_emph_db=0.0, hf_emph_freq=3000.0, hf_extra_db=0.0),
}

AUTOGAIN_TABLE_PATH = Path(__file__).with_name("autogain_table.json")


@dataclass(frozen=True)
class Params:
    warmth: float = 0.4        # 0..1
    input_db: float = 0.0      # -12..+12, gain staging into the curve
    tone: float = 0.0          # -1 (darker) .. +1 (brighter)
    body: float = 0.3          # 0..1
    character: str = "tube"    # tape | tube | console
    low_protect: float = 0.5   # 0..1
    mix: float = 1.0           # 0..1
    auto_gain: bool = True
    output_db: float = 0.0
    oversampling: int = 2      # 1, 2 or 4
    curve_bypass: bool = False # analysis only (not a plugin control): same chain, linear curve

    def with_(self, **kw) -> "Params":
        return replace(self, **kw)


# ----------------------------------------------------------------------------
# Helpers
# ----------------------------------------------------------------------------
def db2lin(db):
    return 10.0 ** (np.asarray(db) / 20.0)


def drive_db(warmth: float, input_db: float = 0.0) -> float:
    return DRIVE_MIN_DB + (DRIVE_MAX_DB - DRIVE_MIN_DB) * warmth ** WARMTH_TAPER + input_db


# ----------------------------------------------------------------------------
# Filters (RBJ cookbook biquads). Shelves with gain g and -g are exact inverses.
# ----------------------------------------------------------------------------
def _shelf(fs, f0, gain_db, slope, high):
    A = 10 ** (gain_db / 40)
    w0 = 2 * np.pi * f0 / fs
    cw, sw = np.cos(w0), np.sin(w0)
    alpha = sw / 2 * np.sqrt((A + 1 / A) * (1 / slope - 1) + 2)
    k = 2 * np.sqrt(A) * alpha
    s = -1 if high else 1   # high shelf = low shelf with the sign of the cos terms flipped
    b = np.array([A * ((A + 1) - s * (A - 1) * cw + k),
                  s * 2 * A * ((A - 1) - s * (A + 1) * cw),
                  A * ((A + 1) - s * (A - 1) * cw - k)])
    a = np.array([(A + 1) + s * (A - 1) * cw + k,
                  -s * 2 * ((A - 1) + s * (A + 1) * cw),
                  (A + 1) + s * (A - 1) * cw - k])
    return b / a[0], a / a[0]


def low_shelf(fs, f0, gain_db, slope=1.0):
    return _shelf(fs, f0, gain_db, slope, high=False)


def high_shelf(fs, f0, gain_db, slope=1.0):
    return _shelf(fs, f0, gain_db, slope, high=True)


def peaking(fs, f0, gain_db, q):
    A = 10 ** (gain_db / 40)
    w0 = 2 * np.pi * f0 / fs
    alpha = np.sin(w0) / (2 * q)
    cw = np.cos(w0)
    b = np.array([1 + alpha * A, -2 * cw, 1 - alpha * A])
    a = np.array([1 + alpha / A, -2 * cw, 1 - alpha / A])
    return b / a[0], a / a[0]


def dc_blocker(fs, f0=DC_BLOCK_FREQ):
    r = np.exp(-2 * np.pi * f0 / fs)
    return np.array([1.0, -1.0]), np.array([1.0, -r])


def apply(ba, x):
    b, a = ba
    return signal.lfilter(b, a, x, axis=-1)


# ----------------------------------------------------------------------------
# Character curves. All satisfy f(0)=0 and f'(0)=1 after biasing, so the
# small-signal gain of shaper(g*x)/g is exactly 1 and drive only changes colour.
# ----------------------------------------------------------------------------
def _tanh(u):
    return np.tanh(u)


def _tanh_d(u):
    return 1.0 - np.tanh(u) ** 2


def _cubic(u):
    # Soft clip: v - 4/27 v^3 on |v|<=1.5 (reaches exactly 1.0 with zero slope), flat beyond.
    v = np.clip(u, -1.5, 1.5)
    return v - (4.0 / 27.0) * v ** 3


def _cubic_d(u):
    v = np.clip(u, -1.5, 1.5)
    return 1.0 - (4.0 / 9.0) * v ** 2


CURVES = {"tanh": (_tanh, _tanh_d), "cubic": (_cubic, _cubic_d)}


def shaper(u, character: str):
    c = CHARACTERS[character]
    f, fd = CURVES[c["curve"]]
    b = c["bias"]
    return (f(u + b) - f(b)) / fd(b)


# ----------------------------------------------------------------------------
# Auto gain: static compensation, calibrated offline by calibrate.py
# ----------------------------------------------------------------------------
_table_cache = None


def _autogain_table():
    global _table_cache
    if _table_cache is None:
        _table_cache = json.loads(AUTOGAIN_TABLE_PATH.read_text()) if AUTOGAIN_TABLE_PATH.exists() else {}
    return _table_cache


def autogain_db(p: Params) -> float:
    t = _autogain_table()
    if not t:
        return 0.0
    ct = t["characters"][p.character]
    d_grid = np.array(t["drive_grid_db"])
    lp_grid = np.array(t["low_protect_grid"])
    rows = np.array(ct["drive_comp_db"])  # [lp][drive]
    d = drive_db(p.warmth, p.input_db)
    per_lp = [np.interp(d, d_grid, r) for r in rows]
    g = np.interp(p.low_protect, lp_grid, per_lp)
    g += ct["body_db_per_unit"] * p.body
    g += (ct["tone_db_pos"] if p.tone >= 0 else ct["tone_db_neg"]) * abs(p.tone)
    return float(g)


# ----------------------------------------------------------------------------
# The chain
# ----------------------------------------------------------------------------
def _saturate(x, fs, p: Params, c):
    """drive -> curve -> /drive, run at fs*oversampling."""
    os_ = p.oversampling
    u = signal.resample_poly(x, os_, 1, axis=-1) if os_ > 1 else x
    g = db2lin(drive_db(p.warmth, p.input_db))
    y = u if p.curve_bypass else shaper(u * g, p.character) / g
    return signal.resample_poly(y, 1, os_, axis=-1) if os_ > 1 else y


def process(x: np.ndarray, fs: float, p: Params) -> np.ndarray:
    x = np.atleast_2d(np.asarray(x, dtype=np.float64))
    c = CHARACTERS[p.character]
    lp_db = -LOW_PROTECT_MAX_DB * p.low_protect
    emph_db = c["hf_emph_db"]

    wet = x
    if lp_db:
        wet = apply(low_shelf(fs, LOW_PROTECT_FREQ, lp_db), wet)
    if emph_db:
        wet = apply(high_shelf(fs, c["hf_emph_freq"], emph_db), wet)

    wet = _saturate(wet, fs, p, c)
    wet = apply(dc_blocker(fs), wet)

    if emph_db:
        wet = apply(high_shelf(fs, c["hf_emph_freq"], -emph_db), wet)
    if lp_db:
        wet = apply(low_shelf(fs, LOW_PROTECT_FREQ, -lp_db), wet)

    hf_db = -p.warmth * (HF_ROLLOFF_DB + c["hf_extra_db"])
    if hf_db:
        wet = apply(high_shelf(fs, HF_ROLLOFF_FREQ, hf_db, slope=0.7), wet)
    if p.tone:
        wet = apply(low_shelf(fs, TONE_LOW_FREQ, -TONE_MAX_DB * p.tone, slope=0.5), wet)
        wet = apply(high_shelf(fs, TONE_HIGH_FREQ, TONE_MAX_DB * p.tone, slope=0.5), wet)
    if p.body:
        wet = apply(peaking(fs, BODY_FREQ, BODY_MAX_DB * p.body, BODY_Q), wet)

    gain_db = autogain_db(p) if p.auto_gain else 0.0
    wet = wet * db2lin(gain_db)

    out = (1.0 - p.mix) * x + p.mix * wet
    return out * db2lin(p.output_db)
