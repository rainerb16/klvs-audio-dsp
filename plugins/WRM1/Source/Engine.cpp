#include "Engine.h"
#include "AutoGainTable.h"

namespace wrm1
{
namespace
{
    // ---- docs/WRM1_DSP.md constants (must match dsp-lab/wrm1.py) ----------------------------
    constexpr double driveMinDb = -4.0, driveMaxDb = 14.0;
    constexpr double lowProtectFreq = 100.0, lowProtectMaxDb = 12.0;
    constexpr double hfRolloffFreq = 7000.0, hfRolloffDb = 2.0, hfRolloffSlope = 0.7;
    constexpr double toneLowFreq = 500.0, toneHighFreq = 2000.0, toneMaxDb = 2.0, toneSlope = 0.5;
    constexpr double bodyFreq = 180.0, bodyQ = 0.8, bodyMaxDb = 3.0;
    constexpr double dcBlockFreq = 10.0;
    constexpr double emphasisFreq = 3000.0;

    struct CharacterSpec
    {
        rainr::dsp::CurveShape shape;
        double bias, emphasisDb, hfExtraDb;
    };

    // Index = Character parameter (0 Tape, 1 Tube, 2 Console)
    constexpr CharacterSpec characters[3] = {
        { rainr::dsp::CurveShape::tanh, 0.15, 4.0, 1.5 },
        { rainr::dsp::CurveShape::tanh, 0.25, 0.0, 0.5 },
        { rainr::dsp::CurveShape::cubic, 0.06, 0.0, 0.0 },
    };

    // ---- smoothing times (spec section 3) -----------------------------------------------------
    constexpr double filterRampSeconds = 0.030, driveRampSeconds = 0.030, characterFadeSeconds = 0.030;
    constexpr double autoGainRampSeconds = 0.050, mixRampSeconds = 0.020;
    constexpr int subBlockSize = 16;


    double interpRow (const double* row, double driveDb)
    {
        using namespace autogain;
        const double pos = std::clamp ((driveDb - driveGridStartDb) / driveGridStepDb, 0.0, double (driveGridSize - 1));
        const int i = std::min ((int) pos, driveGridSize - 2);
        const double t = pos - i;
        return row[i] * (1.0 - t) + row[i + 1] * t;
    }
} // namespace

double driveDbFor (float warmth, float inputDb) noexcept
{
    return driveMinDb + (driveMaxDb - driveMinDb) * warmth + inputDb;
}

double transferCurve (int character, double driveDb, double x) noexcept
{
    const auto& spec = characters[std::clamp (character, 0, 2)];
    const rainr::dsp::BiasedCurve curve (spec.shape, spec.bias);
    const double g = std::pow (10.0, driveDb / 20.0);
    return curve (g * x) / g;
}

double autoGainDb (int character, double driveDb, double lowProtect, double body, double tone) noexcept
{
    const auto& table = autogain::tables[std::clamp (character, 0, 2)];
    const double lp = std::clamp (lowProtect, 0.0, 1.0) * 2.0; // grid 0, 0.5, 1 -> 0, 1, 2
    const int row = std::min ((int) lp, 1);
    const double t = lp - row;
    double g = interpRow (table.driveCompDb[row], driveDb) * (1.0 - t) + interpRow (table.driveCompDb[row + 1], driveDb) * t;
    g += table.bodyDbPerUnit * body;
    g += (tone >= 0.0 ? table.toneDbPos : table.toneDbNeg) * std::abs (tone);
    return g;
}

void Engine::prepare (double newSampleRate, int maximumBlockSize, int numChannels)
{
    sampleRate = newSampleRate;
    maxBlock = std::max (1, maximumBlockSize);
    numCh = std::clamp (numChannels, 1, maxChannels);

    wet.setSize (numCh, maxBlock);
    dry.setSize (numCh, maxBlock);
    subBlocks.resize ((size_t) (maxBlock / subBlockSize + 1));

    for (int i = 1; i <= 2; ++i)
    {
        oversamplers[(size_t) i] = std::make_unique<juce::dsp::Oversampling<float>> (
            (size_t) numCh, (size_t) i, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
            true /* max quality */, false /* latency rounding is done exactly in updateLatency() */);
        oversamplers[(size_t) i]->initProcessing ((size_t) maxBlock);
    }

    dryDelay.setMaximumDelayInSamples (1024);
    dryDelay.prepare ({ sampleRate, (juce::uint32) maxBlock, (juce::uint32) numCh });

    for (int i = 0; i < 3; ++i)
        curves[(size_t) i] = rainr::dsp::BiasedCurve (characters[i].shape, characters[i].bias);

    for (auto* s : { &driveDbHost, &lowProtectDb, &emphasisDb, &hfDb, &toneDb, &bodyDb })
        s->reset (sampleRate, filterRampSeconds);
    autoGainDbSm.reset (sampleRate, autoGainRampSeconds);
    mixSm.reset (sampleRate, mixRampSeconds);
    outputGain.reset (sampleRate, mixRampSeconds);

    dcBlocker.prepare (sampleRate, dcBlockFreq);
    for (auto* f : { &preLowProtect, &preEmphasis, &postEmphasis, &postLowProtect, &hfRolloff, &toneLow, &toneHigh, &bodyBell })
        f->reset();

    osIndex = -1; // force updateLatency() on the next setSettings()
    setSettings (target);
    snapToTargets();
}

void Engine::updateLatency() noexcept
{
    const double factor = double (1 << osIndex);
    driveDb.reset (sampleRate * factor, driveRampSeconds);
    fadeLength = std::max (1, (int) (characterFadeSeconds * sampleRate * factor));
    fadeRemaining = 0;

    // JUCE's own integer-latency option rounds with an allpass whose delay varies with frequency.
    // Instead, pad with whole samples at the oversampled rate, where the fractional part of the
    // latency is a whole number of samples. The result is linear-phase and exactly integer.
    latency = 0;
    latencyPad = 0;
    if (osIndex > 0)
    {
        const double raw = oversamplers[(size_t) osIndex]->getLatencyInSamples();
        latency = (int) std::ceil (raw - 1e-6);
        latencyPad = (int) std::lround ((latency - raw) * factor);
        jassert (std::abs ((latency - raw) * factor - latencyPad) < 1e-6); // padding must be exact
        oversamplers[(size_t) osIndex]->reset();
    }
    padBuffer = {};
    padPos = 0;
    dryDelay.reset();
    dryDelay.setDelay ((float) latency);
}

void Engine::setSettings (const Settings& s) noexcept
{
    target = s;
    const int ch = std::clamp (s.character, 0, 2);
    const auto& spec = characters[ch];
    const auto drive = (float) driveDbFor (s.warmth, s.inputDb);

    if (const int os = std::clamp (s.oversampling, 0, 2); os != osIndex)
    {
        // Not automatable (latency changes); a switch resets the oversampler and dry delay.
        osIndex = os;
        updateLatency();
    }

    driveDb.setTargetValue (drive);
    driveDbHost.setTargetValue (drive);
    lowProtectDb.setTargetValue ((float) (-lowProtectMaxDb * s.lowProtect));
    emphasisDb.setTargetValue ((float) spec.emphasisDb);
    hfDb.setTargetValue ((float) (-s.warmth * (hfRolloffDb + spec.hfExtraDb)));
    toneDb.setTargetValue ((float) (toneMaxDb * s.tone));
    bodyDb.setTargetValue ((float) (bodyMaxDb * s.body));
    mixSm.setTargetValue (s.mix);
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (s.outputDb, -1000.0f));

    if (ch != character)
    {
        previousCharacter = character;
        character = ch;
        fadeRemaining = fadeLength;
    }
}

void Engine::snapToTargets() noexcept
{
    for (auto* s : { &driveDb, &driveDbHost, &lowProtectDb, &emphasisDb, &hfDb, &toneDb, &bodyDb, &mixSm, &outputGain })
        s->setCurrentAndTargetValue (s->getTargetValue());
    previousCharacter = character;
    fadeRemaining = 0;
    const SubBlock now { lowProtectDb.getTargetValue(), emphasisDb.getTargetValue(), hfDb.getTargetValue(),
                         toneDb.getTargetValue(), bodyDb.getTargetValue() };
    autoGainDbSm.setCurrentAndTargetValue (autoGainTarget (now));
    forceCoeffs = true;
}

float Engine::autoGainTarget (const SubBlock& sb) const noexcept
{
    if (! target.autoGain)
        return 0.0f;
    // Derived from the smoothed filter values so Auto Gain follows the same ramps (spec 3).
    return (float) autoGainDb (character, driveDbHost.getCurrentValue(), -sb.lowProtectDb / lowProtectMaxDb,
                               sb.bodyDb / bodyMaxDb, sb.toneDb / toneMaxDb);
}

void Engine::process (juce::AudioBuffer<float>& buffer) noexcept
{
    const int channels = std::min ({ buffer.getNumChannels(), numCh, maxChannels });
    std::array<float*, maxChannels> ptrs {};
    for (int start = 0; start < buffer.getNumSamples(); start += maxBlock)
    {
        const int n = std::min (maxBlock, buffer.getNumSamples() - start);
        for (int c = 0; c < channels; ++c)
            ptrs[(size_t) c] = buffer.getWritePointer (c, start);
        processChunk (ptrs.data(), channels, n);
    }
}

void Engine::saturate (juce::dsp::AudioBlock<float>& block) noexcept
{
    const int n = (int) block.getNumSamples();
    const int channels = (int) block.getNumChannels();
    for (int i = 0; i < n; ++i)
    {
        const float dB = driveDb.getNextValue();
        if (! juce::exactlyEqual (dB, lastDriveDb))
        {
            lastDriveDb = dB;
            lastDriveGain = juce::Decibels::decibelsToGain (dB, -1000.0f);
        }
        const double g = lastDriveGain, invG = 1.0 / g;

        double fade = 1.0; // weight of the current character
        if (fadeRemaining > 0)
            fade = 1.0 - double (fadeRemaining--) / fadeLength;

        const auto& cur = curves[(size_t) character];
        const auto& prev = curves[(size_t) previousCharacter];
        const int writePos = padPos;
        const int readPos = (padPos - latencyPad) & 7;
        padPos = (padPos + 1) & 7;
        for (int c = 0; c < channels; ++c)
        {
            auto* x = block.getChannelPointer ((size_t) c);
            if (latencyPad > 0)
            {
                padBuffer[(size_t) c][(size_t) writePos] = x[i];
                x[i] = padBuffer[(size_t) c][(size_t) readPos];
            }
            const double u = x[i] * g;
            double y = cur (u);
            if (fade < 1.0)
                y = fade * y + (1.0 - fade) * prev (u);
            x[i] = (float) (y * invG);
        }
    }
}

void Engine::processChunk (float* const* io, int channels, int n) noexcept
{
    for (int c = 0; c < channels; ++c)
    {
        wet.copyFrom (c, 0, io[c], n);
        dry.copyFrom (c, 0, io[c], n);
    }

    // 1. Capture smoothed filter values per sub-block; pre-emphasis.
    const int numSub = (n + subBlockSize - 1) / subBlockSize;
    for (int k = 0; k < numSub; ++k)
    {
        const int pos = k * subBlockSize, len = std::min (subBlockSize, n - pos);
        auto& sb = subBlocks[(size_t) k];
        sb = { lowProtectDb.getCurrentValue(), emphasisDb.getCurrentValue(), hfDb.getCurrentValue(),
               toneDb.getCurrentValue(), bodyDb.getCurrentValue() };
        for (auto* s : { &lowProtectDb, &emphasisDb, &hfDb, &toneDb, &bodyDb })
            s->skip (len);

        if (forceCoeffs || ! juce::exactlyEqual (sb.lowProtectDb, lastPre.lowProtectDb))
            preLowProtect.setCoefficients (rainr::dsp::rbj::lowShelf (sampleRate, lowProtectFreq, sb.lowProtectDb));
        if (forceCoeffs || ! juce::exactlyEqual (sb.emphasisDb, lastPre.emphasisDb))
            preEmphasis.setCoefficients (rainr::dsp::rbj::highShelf (sampleRate, emphasisFreq, sb.emphasisDb));
        lastPre = sb;

        for (int c = 0; c < channels; ++c)
        {
            preLowProtect.process (wet.getWritePointer (c, pos), len, c);
            preEmphasis.process (wet.getWritePointer (c, pos), len, c);
        }
    }

    // 2. Drive -> curve -> /drive, oversampled.
    juce::dsp::AudioBlock<float> block (wet.getArrayOfWritePointers(), (size_t) channels, (size_t) n);
    if (osIndex > 0)
    {
        auto& os = *oversamplers[(size_t) osIndex];
        auto up = os.processSamplesUp (block);
        saturate (up);
        os.processSamplesDown (block);
    }
    else
    {
        saturate (block);
    }

    // 3. DC blocker, de-emphasis, HF rolloff, Tone, Body, Auto Gain.
    for (int c = 0; c < channels; ++c)
        dcBlocker.process (wet.getWritePointer (c), n, c);

    for (int k = 0; k < numSub; ++k)
    {
        const int pos = k * subBlockSize, len = std::min (subBlockSize, n - pos);
        const auto& sb = subBlocks[(size_t) k];

        if (forceCoeffs || ! juce::exactlyEqual (sb.emphasisDb, lastPost.emphasisDb))
            postEmphasis.setCoefficients (rainr::dsp::rbj::highShelf (sampleRate, emphasisFreq, -sb.emphasisDb));
        if (forceCoeffs || ! juce::exactlyEqual (sb.lowProtectDb, lastPost.lowProtectDb))
            postLowProtect.setCoefficients (rainr::dsp::rbj::lowShelf (sampleRate, lowProtectFreq, -sb.lowProtectDb));
        if (forceCoeffs || ! juce::exactlyEqual (sb.hfDb, lastPost.hfDb))
            hfRolloff.setCoefficients (rainr::dsp::rbj::highShelf (sampleRate, hfRolloffFreq, sb.hfDb, hfRolloffSlope));
        if (forceCoeffs || ! juce::exactlyEqual (sb.toneDb, lastPost.toneDb))
        {
            toneLow.setCoefficients (rainr::dsp::rbj::lowShelf (sampleRate, toneLowFreq, -sb.toneDb, toneSlope));
            toneHigh.setCoefficients (rainr::dsp::rbj::highShelf (sampleRate, toneHighFreq, sb.toneDb, toneSlope));
        }
        if (forceCoeffs || ! juce::exactlyEqual (sb.bodyDb, lastPost.bodyDb))
            bodyBell.setCoefficients (rainr::dsp::rbj::peaking (sampleRate, bodyFreq, sb.bodyDb, bodyQ));
        lastPost = sb;
        forceCoeffs = false;

        for (int c = 0; c < channels; ++c)
        {
            auto* x = wet.getWritePointer (c, pos);
            for (auto* f : { &postEmphasis, &postLowProtect, &hfRolloff, &toneLow, &toneHigh, &bodyBell })
                f->process (x, len, c);
        }

        autoGainDbSm.setTargetValue (autoGainTarget (sb));
        driveDbHost.skip (len);
        for (int i = pos; i < pos + len; ++i)
        {
            const float dB = autoGainDbSm.getNextValue();
            if (! juce::exactlyEqual (dB, lastAutoGainDb))
            {
                lastAutoGainDb = dB;
                lastAutoGain = juce::Decibels::decibelsToGain (dB, -1000.0f);
            }
            for (int c = 0; c < channels; ++c)
                wet.getWritePointer (c)[i] *= lastAutoGain;
        }
    }

    // 4. Mix with the latency-aligned dry signal, then Output.
    for (int i = 0; i < n; ++i)
    {
        const float m = mixSm.getNextValue();
        const float og = outputGain.getNextValue();
        for (int c = 0; c < channels; ++c)
        {
            float d = dry.getSample (c, i);
            if (latency > 0)
            {
                dryDelay.pushSample (c, d);
                d = dryDelay.popSample (c);
            }
            io[c][i] = ((1.0f - m) * d + m * wet.getSample (c, i)) * og;
        }
    }
}
} // namespace wrm1
