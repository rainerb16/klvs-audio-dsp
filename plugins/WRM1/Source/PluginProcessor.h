#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Engine.h"

namespace wrm1
{
class Processor final : public juce::AudioProcessor
{
public:
    Processor();

    void prepareToPlay (double sampleRate, int maximumBlockSize) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState state;

    /** Peak levels since the last call, for the meters (read and reset by the editor). */
    std::array<float, 2> takeInputPeaks() noexcept { return take (inputPeak); }
    std::array<float, 2> takeOutputPeaks() noexcept { return take (outputPeak); }

private:
    using Peaks = std::array<std::atomic<float>, 2>;
    static std::array<float, 2> take (Peaks& p) noexcept { return { p[0].exchange (0.0f), p[1].exchange (0.0f) }; }
    static void record (Peaks& p, const juce::AudioBuffer<float>& b) noexcept;

    Peaks inputPeak {}, outputPeak {};

    Settings readSettings() const noexcept;

    Engine engine;

    // Lock-free views of the parameter values, read on the audio thread.
    std::atomic<float>* pInput = nullptr;
    std::atomic<float>* pWarmth = nullptr;
    std::atomic<float>* pTone = nullptr;
    std::atomic<float>* pBody = nullptr;
    std::atomic<float>* pCharacter = nullptr;
    std::atomic<float>* pLowProtect = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pAutoGain = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pOversampling = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Processor)
};
} // namespace wrm1
