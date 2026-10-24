#pragma once

#include <rainr_ui/rainr_ui.h>

#include "PluginProcessor.h"

namespace wrm1
{
/** A knob with its name and live value underneath, bound to one parameter. */
class KnobView : public juce::Component
{
public:
    enum class Style { small, smallBipolar, big };
    KnobView (juce::AudioProcessorValueTreeState&, const char* paramId, const juce::String& name, Style);

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int smallWidth = 74, smallHeight = 86, knobSize = 52;

private:
    juce::Slider slider;
    juce::RangedAudioParameter& param;
    juce::String name;
    bool big;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateScope();

    Processor& pluginProcessor;
    rainr::ui::LookAndFeel lookAndFeel;
    juce::Image lockup;

    KnobView input, warmth, output, tone, body, lowProtect, mix;
    rainr::ui::SegmentedControl character, oversampling;
    juce::ToggleButton autoGain { "Auto Gain" };
    juce::AudioProcessorValueTreeState::ButtonAttachment autoGainAttachment;
    rainr::ui::LevelMeter inputMeter, outputMeter;
    rainr::ui::TransferScope scope;

    std::array<float, 7> lastScopeKey {};
    double lastTimerMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Editor)
};
} // namespace wrm1
