#pragma once

namespace rainr::ui
{
/** A row of mutually exclusive buttons in a recessed well, bound to a choice parameter.
    Follows host automation and undo through juce::ParameterAttachment. */
class SegmentedControl : public juce::Component
{
public:
    SegmentedControl (juce::RangedAudioParameter& parameter, const juce::StringArray& labels);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Segment : public juce::Button
    {
    public:
        explicit Segment (const juce::String& text) : juce::Button (text) {}
        void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    };

    void select (int index);

    juce::OwnedArray<Segment> segments;
    juce::ParameterAttachment attachment;
    int selected = 0;
};
} // namespace rainr::ui
