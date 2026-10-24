#pragma once

namespace rainr::ui
{
/** Saturation display: the transfer curve (glowing) against the straight "no effect" line,
    five harmonic bars (H2 highlighted) and two readouts. The plugin computes the data; this only
    draws it, so it's reusable by any saturating plugin. */
class TransferScope : public juce::Component
{
public:
    static constexpr int curvePoints = 121;

    struct Data
    {
        std::array<float, curvePoints> curve {};   // output for input -1..+1, evenly spaced
        std::array<float, 5> harmonicsDb {};       // H1..H5 relative to H1
        juce::String leftLabel, leftValue, rightLabel, rightValue;
    };

    void setData (const Data&);
    void paint (juce::Graphics&) override;

private:
    Data data;
};
} // namespace rainr::ui
