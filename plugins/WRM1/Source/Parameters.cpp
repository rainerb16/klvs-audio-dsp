#include "Parameters.h"

namespace wrm1::params
{
namespace
{
    juce::ParameterID id (const char* name) { return { name, firstVersionHint }; }

    // Display text and the inverse parser hosts use for typed values ("50 %" -> 0.5).
    juce::String percent (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
    float percentFromText (const juce::String& t) { return t.retainCharacters ("0123456789.-").getFloatValue() / 100.0f; }

    juce::String decibels (float v, int)
    {
        return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " dB";
    }

    float toneFromText (const juce::String& t)
    {
        const float amount = t.retainCharacters ("0123456789.").getFloatValue() / 100.0f;
        if (t.containsIgnoreCase ("dark") || t.trim().startsWithChar ('-'))
            return -amount;
        return amount;
    }

    juce::String toneText (float v, int)
    {
        const auto amount = juce::roundToInt (std::abs (v) * 100.0f);
        if (amount == 0)
            return "0";
        return (v < 0.0f ? "Dark " : "Bright ") + juce::String (amount);
    }

    std::unique_ptr<juce::AudioParameterFloat> unitParam (const char* name, const juce::String& label, float def)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            id (name), label, juce::NormalisableRange<float> (0.0f, 1.0f), def,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (percent).withValueFromStringFunction (percentFromText));
    }

    std::unique_ptr<juce::AudioParameterFloat> gainParam (const char* name, const juce::String& label)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            id (name), label, juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction (decibels)
                .withValueFromStringFunction ([] (const juce::String& t) { return t.retainCharacters ("0123456789.-").getFloatValue(); })
                .withLabel ("dB"));
    }
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (gainParam (input, "Input"));
    layout.add (unitParam (warmth, "Warmth", 0.40f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        id (tone), "Tone", juce::NormalisableRange<float> (-1.0f, 1.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (toneText).withValueFromStringFunction (toneFromText)));
    layout.add (unitParam (body, "Body", 0.30f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (id (character), "Character", characterChoices, 1));
    layout.add (unitParam (lowProtect, "Low Protect", 0.50f));
    layout.add (unitParam (mix, "Mix", 1.00f));
    layout.add (std::make_unique<juce::AudioParameterBool> (id (autoGain), "Auto Gain", true));
    layout.add (gainParam (output, "Output"));
    // Changing oversampling changes latency, which hosts can't follow mid-playback: not automatable.
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        id (oversampling), "Oversampling", oversamplingChoices, 1,
        juce::AudioParameterChoiceAttributes().withAutomatable (false)));

    return layout;
}
} // namespace wrm1::params
