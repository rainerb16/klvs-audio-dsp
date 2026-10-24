#include "PluginProcessor.h"
#include "Parameters.h"
#include "PluginEditor.h"

namespace wrm1
{
namespace
{
    // Bumped only if the saved-state format itself changes (not when parameters are added),
    // so setStateInformation can migrate old projects.
    constexpr int stateFormatVersion = 1;
} // namespace

Processor::Processor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "WRM1", params::createLayout())
{
    pInput = state.getRawParameterValue (params::input);
    pWarmth = state.getRawParameterValue (params::warmth);
    pTone = state.getRawParameterValue (params::tone);
    pBody = state.getRawParameterValue (params::body);
    pCharacter = state.getRawParameterValue (params::character);
    pLowProtect = state.getRawParameterValue (params::lowProtect);
    pMix = state.getRawParameterValue (params::mix);
    pAutoGain = state.getRawParameterValue (params::autoGain);
    pOutput = state.getRawParameterValue (params::output);
    pOversampling = state.getRawParameterValue (params::oversampling);
}

Settings Processor::readSettings() const noexcept
{
    Settings s;
    s.inputDb = pInput->load();
    s.warmth = pWarmth->load();
    s.tone = pTone->load();
    s.body = pBody->load();
    s.character = juce::roundToInt (pCharacter->load());
    s.lowProtect = pLowProtect->load();
    s.mix = pMix->load();
    s.autoGain = pAutoGain->load() >= 0.5f;
    s.outputDb = pOutput->load();
    s.oversampling = juce::roundToInt (pOversampling->load());
    return s;
}

void Processor::prepareToPlay (double sampleRate, int maximumBlockSize)
{
    // Everything that allocates happens here, never in processBlock.
    engine.prepare (sampleRate, maximumBlockSize, getTotalNumOutputChannels());
    engine.setSettings (readSettings());
    engine.snapToTargets();
    setLatencySamples (engine.getLatencySamples());
}

bool Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    record (inputPeak, buffer);
    engine.setSettings (readSettings());
    engine.process (buffer);
    record (outputPeak, buffer);

    // Oversampling changed: tell the host so it can re-align the track.
    if (engine.getLatencySamples() != getLatencySamples())
        setLatencySamples (engine.getLatencySamples());
}

void Processor::record (Peaks& peaks, const juce::AudioBuffer<float>& buffer) noexcept
{
    // Keep the highest peak until the editor collects it; a mono signal shows on both meters.
    for (int c = 0; c < 2; ++c)
    {
        const int src = std::min (c, buffer.getNumChannels() - 1);
        const float level = src >= 0 ? buffer.getMagnitude (src, 0, buffer.getNumSamples()) : 0.0f;
        float prev = peaks[(size_t) c].load (std::memory_order_relaxed);
        while (level > prev && ! peaks[(size_t) c].compare_exchange_weak (prev, level, std::memory_order_relaxed))
        {
        }
    }
}

juce::AudioProcessorEditor* Processor::createEditor()
{
    return new Editor (*this);
}

void Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto tree = state.copyState();
    tree.setProperty ("stateFormatVersion", stateFormatVersion, nullptr);
    tree.setProperty ("pluginVersion", RAINR_PLUGIN_VERSION, nullptr);
    if (auto xml = tree.createXml())
        copyXmlToBinary (*xml, destData);
}

void Processor::setStateInformation (const void* data, int sizeInBytes)
{
    // Unknown parameters are ignored and missing ones keep their defaults, so projects saved by
    // older or newer versions still open.
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (state.state.getType()))
        state.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace wrm1

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new wrm1::Processor();
}
