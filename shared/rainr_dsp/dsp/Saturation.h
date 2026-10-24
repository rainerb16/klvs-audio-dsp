#pragma once

#include <algorithm>
#include <cmath>

namespace rainr::dsp
{
enum class CurveShape
{
    tanh,  // f(u) = tanh(u)
    cubic  // f(u) = v - 4/27 v^3, v = clamp(u, -1.5, 1.5): reaches exactly +-1 with zero slope
};

/** Biased waveshaper: curve(u) = (f(u + b) - f(b)) / f'(b).
    curve(0) = 0 and curve'(0) = 1 for any bias, so the small-signal gain is exactly 1; the bias
    makes the curve asymmetric, which produces even (2nd) harmonics. */
class BiasedCurve
{
public:
    BiasedCurve() = default;

    BiasedCurve (CurveShape s, double biasAmount) noexcept
        : shape (s), bias (biasAmount), offset (base (s, biasAmount)), invSlope (1.0 / slope (s, biasAmount))
    {
    }

    double operator() (double u) const noexcept { return (base (shape, u + bias) - offset) * invSlope; }

    static double base (CurveShape s, double u) noexcept
    {
        if (s == CurveShape::tanh)
            return std::tanh (u);
        const double v = std::clamp (u, -1.5, 1.5);
        return v - (4.0 / 27.0) * v * v * v;
    }

    static double slope (CurveShape s, double u) noexcept
    {
        if (s == CurveShape::tanh)
        {
            const double t = std::tanh (u);
            return 1.0 - t * t;
        }
        const double v = std::clamp (u, -1.5, 1.5);
        return 1.0 - (4.0 / 9.0) * v * v;
    }

private:
    CurveShape shape = CurveShape::tanh;
    double bias = 0.0, offset = 0.0, invSlope = 1.0;
};
} // namespace rainr::dsp
