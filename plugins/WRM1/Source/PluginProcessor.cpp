#include "PluginProcessor.h"
#include "Parameters.h"

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
}

void Processor::prepareToPlay (double, int)
{
    // Phase 2 step 3: size every buffer, smoother and oversampler here, never in processBlock.
    setLatencySamples (0);
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

    // Step 1: clean pass-through. The WRM1 chain is ported in Phase 2 step 3.
    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* Processor::createEditor()
{
    // Placeholder until the real UI (Phase 2 step 4): JUCE's stock editor lists every parameter.
    return new juce::GenericAudioProcessorEditor (*this);
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
