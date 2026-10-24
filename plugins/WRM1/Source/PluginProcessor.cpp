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
    startTimerHz (10);
}

void Processor::timerCallback()
{
    if (const int l = engineLatency.load(); l != getLatencySamples())
        setLatencySamples (l);
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
    engineLatency = engine.getLatencySamples();
    setLatencySamples (engineLatency);
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

    engineLatency.store (engine.getLatencySamples(), std::memory_order_relaxed);
}

void Processor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // Bypassed audio must keep the reported latency, or the host's delay compensation would shift
    // this track early against the others (JUCE's default bypass assumes zero latency).
    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
    engine.processBypassed (buffer);
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
    // Projects saved by older or newer versions must still open: unknown parameters are ignored,
    // and parameters the project doesn't know about load at their defaults. (APVTS on its own
    // would keep whatever value they currently have.) "PARAM"/"id"/"value" is APVTS's format.
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (state.state.getType()))
        return;

    auto tree = juce::ValueTree::fromXml (*xml);
    const int formatVersion = tree.getProperty ("stateFormatVersion", 1);
    jassert (formatVersion <= stateFormatVersion); // newer format: load what we understand
    juce::ignoreUnused (formatVersion);            // no migrations exist yet (format 1 only)

    for (auto* p : getParameters())
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! tree.getChildWithProperty ("id", param->paramID).isValid())
                tree.appendChild (juce::ValueTree ("PARAM", { { "id", param->paramID },
                                                              { "value", param->convertFrom0to1 (param->getDefaultValue()) } }),
                                  nullptr);
    state.replaceState (tree);
}
} // namespace wrm1

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new wrm1::Processor();
}
