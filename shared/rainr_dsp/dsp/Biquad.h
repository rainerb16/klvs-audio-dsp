#pragma once

#include <array>
#include <cmath>
#include <numbers>

namespace rainr::dsp
{
/** Biquad coefficients, normalised so that a0 == 1. */
struct BiquadCoeffs
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
};

/** RBJ Audio EQ Cookbook designs. A shelf with +g dB and the same shelf with -g dB are exact
    inverses (numerator and denominator swap), which pre/de-emphasis pairs rely on. */
namespace rbj
{
    inline BiquadCoeffs shelf (double fs, double f0, double gainDb, double slope, bool high) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * std::numbers::pi * f0 / fs;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / 2.0 * std::sqrt ((A + 1.0 / A) * (1.0 / slope - 1.0) + 2.0);
        const double k = 2.0 * std::sqrt (A) * alpha;
        const double s = high ? -1.0 : 1.0; // a high shelf is a low shelf with the cos terms' sign flipped

        const double b0 = A * ((A + 1.0) - s * (A - 1.0) * cw + k);
        const double b1 = s * 2.0 * A * ((A - 1.0) - s * (A + 1.0) * cw);
        const double b2 = A * ((A + 1.0) - s * (A - 1.0) * cw - k);
        const double a0 = (A + 1.0) + s * (A - 1.0) * cw + k;
        const double a1 = -s * 2.0 * ((A - 1.0) + s * (A + 1.0) * cw);
        const double a2 = (A + 1.0) + s * (A - 1.0) * cw - k;
        return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
    }

    inline BiquadCoeffs lowShelf (double fs, double f0, double gainDb, double slope = 1.0) noexcept
    {
        return shelf (fs, f0, gainDb, slope, false);
    }

    inline BiquadCoeffs highShelf (double fs, double f0, double gainDb, double slope = 1.0) noexcept
    {
        return shelf (fs, f0, gainDb, slope, true);
    }

    inline BiquadCoeffs peaking (double fs, double f0, double gainDb, double q) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * std::numbers::pi * f0 / fs;
        const double alpha = std::sin (w0) / (2.0 * q);
        const double cw = std::cos (w0);
        const double a0 = 1.0 + alpha / A;
        return { (1.0 + alpha * A) / a0, -2.0 * cw / a0, (1.0 - alpha * A) / a0, -2.0 * cw / a0, (1.0 - alpha / A) / a0 };
    }
} // namespace rbj

/** Transposed direct form II biquad, double precision state, for up to MaxChannels channels.
    Never allocates. */
template <int MaxChannels = 2>
class Biquad
{
public:
    void setCoefficients (const BiquadCoeffs& c) noexcept { coeffs = c; }

    void reset() noexcept { state = {}; }

    void process (float* data, int numSamples, int channel) noexcept
    {
        auto& [z1, z2] = state[(size_t) channel];
        const auto [b0, b1, b2, a1, a2] = coeffs;
        for (int i = 0; i < numSamples; ++i)
        {
            const double x = data[i];
            const double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            data[i] = (float) y;
        }
    }

private:
    BiquadCoeffs coeffs;
    std::array<std::array<double, 2>, MaxChannels> state {};
};

/** One-pole DC blocker: y[n] = x[n] - x[n-1] + R * y[n-1]. */
template <int MaxChannels = 2>
class DcBlocker
{
public:
    void prepare (double sampleRate, double cutoffHz) noexcept
    {
        r = std::exp (-2.0 * std::numbers::pi * cutoffHz / sampleRate);
        reset();
    }

    void reset() noexcept { state = {}; }

    void process (float* data, int numSamples, int channel) noexcept
    {
        auto& [x1, y1] = state[(size_t) channel];
        for (int i = 0; i < numSamples; ++i)
        {
            const double x = data[i];
            const double y = x - x1 + r * y1;
            x1 = x;
            y1 = y;
            data[i] = (float) y;
        }
    }

private:
    double r = 0.0;
    std::array<std::array<double, 2>, MaxChannels> state {};
};
} // namespace rainr::dsp
