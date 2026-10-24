#pragma once

#include <juce_dsp/juce_dsp.h>
#include <rainr_dsp/rainr_dsp.h>

namespace wrm1
{
/** Plain parameter values, in the units of docs/WRM1_DSP.md section 1. */
struct Settings
{
    float inputDb = 0.0f;
    float warmth = 0.40f;
    float tone = 0.0f;
    float body = 0.30f;
    int character = 1;      // 0 Tape, 1 Tube, 2 Console
    float lowProtect = 0.50f;
    float mix = 1.0f;
    bool autoGain = true;
    float outputDb = 0.0f;
    int oversampling = 1;   // 0 Off, 1 2x, 2 4x
};

/** Auto Gain in dB (docs/WRM1_DSP.md 2.9). */
double autoGainDb (int character, double driveDb, double lowProtect, double body, double tone) noexcept;

/** Drive in dB for Warmth and Input (docs/WRM1_DSP.md 2.1). */
double driveDbFor (float warmth, float inputDb) noexcept;

/** The static transfer curve, curve(g x) / g, for displays (docs/WRM1_DSP.md 2.1-2.2). */
double transferCurve (int character, double driveDb, double x) noexcept;

/** The WRM1 signal chain (docs/WRM1_DSP.md). Kept apart from the AudioProcessor so the tests can
    run it directly.

    Threading: prepare() allocates and must not run on the audio thread. setSettings(), process()
    and snapToTargets() never allocate. */
class Engine
{
public:
    static constexpr int maxChannels = 2;

    void prepare (double sampleRate, int maximumBlockSize, int numChannels);

    /** New parameter values; changes are smoothed (spec section 3). */
    void setSettings (const Settings& s) noexcept;

    /** Jump straight to the current settings with no smoothing (after prepare, and in tests). */
    void snapToTargets() noexcept;

    /** Processes in place. Any block size; channels beyond maxChannels are left untouched.
        Before prepare() the audio passes through unchanged. */
    void process (juce::AudioBuffer<float>& buffer) noexcept;

    /** Host bypass: the dry signal, delayed by the same latency as process(), so bypassing doesn't
        shift the track in time. Shares the dry delay with process() so toggling is seamless. */
    void processBypassed (juce::AudioBuffer<float>& buffer) noexcept;

    /** Oversampler latency; the dry path is delayed by the same amount. */
    int getLatencySamples() const noexcept { return latency; }

private:
    using Smoothed = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;
    using Filter = rainr::dsp::Biquad<maxChannels>;

    /** Smoothed values captured at the start of each 16-sample sub-block. Pre- and post-filters
        read the same entry, so emphasis pairs stay exact inverses while they move. */
    struct SubBlock
    {
        float lowProtectDb, emphasisDb, hfDb, toneDb, bodyDb;
    };

    void processChunk (float* const* channels, int numChannels, int numSamples) noexcept;
    void saturate (juce::dsp::AudioBlock<float>& block) noexcept;
    void updateLatency() noexcept;
    void resetState() noexcept;
    void startPendingCharacterFade() noexcept;
    float autoGainTarget (const SubBlock& sb) const noexcept;

    static constexpr int maxDryDelay = 1024;

    bool prepared = false;
    double sampleRate = 48000.0;
    int maxBlock = 0, numCh = 2;

    Settings target;
    int osIndex = -1;
    int latency = 0;
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3> oversamplers; // [0] unused (Off)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 1 };
    juce::AudioBuffer<float> wet, dry;
    std::vector<SubBlock> subBlocks;

    // Smoothers (spec section 3)
    Smoothed driveDb;      // at the oversampled rate
    Smoothed driveDbHost;  // same target at the host rate, for Auto Gain
    Smoothed lowProtectDb, emphasisDb, hfDb, toneDb, bodyDb;
    Smoothed autoGainDbSm, mixSm, outputGain;
    float lastDriveDb = 1000.0f, lastDriveGain = 1.0f;

    // Whole-sample delay inside the oversampled section that rounds the oversampler's latency up
    // to a whole number of host samples. Exact, unlike a fractional-delay filter (spec section 3).
    int latencyPad = 0, padPos = 0;
    std::array<std::array<float, 8>, maxChannels> padBuffer {};
    float lastAutoGainDb = 1000.0f, lastAutoGain = 1.0f;

    // Character crossfade
    std::array<rainr::dsp::BiasedCurve, 3> curves;
    int character = 1, previousCharacter = 1;
    int requestedCharacter = 1; // a switch during a fade waits for the fade to finish (no jump)
    int fadeRemaining = 0, fadeLength = 1;

    Filter preLowProtect, preEmphasis, postEmphasis, postLowProtect, hfRolloff, toneLow, toneHigh, bodyBell;
    rainr::dsp::DcBlocker<maxChannels> dcBlocker;
    SubBlock lastPre {}, lastPost {};
    bool forceCoeffs = true;
};
} // namespace wrm1
