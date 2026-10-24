#pragma once

namespace rainr::ui
{
/** Segmented LED-style peak meter for up to two channels, in a recessed slot.
    Scale (bottom to top): -48, -24, -12, -6, 0 dBFS at equal spacing, so the useful mix-bus range
    gets most of the height. Feed it with setPeaks() from a UI timer. */
class LevelMeter : public juce::Component
{
public:
    static constexpr int barWidth = 10, barGap = 6;

    /** Linear peak values since the last call (0..1+). Applies the fall-off and repaints. */
    void setPeaks (float left, float right, double secondsSinceLast);

    /** Position of a dB value from the bottom of the meter, 0..1. */
    static float positionFor (float dB) noexcept;

    void paint (juce::Graphics&) override;

private:
    std::array<float, 2> displayDb { -100.0f, -100.0f };
};
} // namespace rainr::ui
