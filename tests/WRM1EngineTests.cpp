// WRM1 engine tests: the C++ engine against the Python reference (tests/golden, written by
// dsp-lab/make_golden.py), plus checks Python can't make (latency, block sizes, stability).
// Exit code 0 = all passed.

#include <juce_audio_formats/juce_audio_formats.h>

#include "Engine.h"

namespace
{
int failures = 0;

void check (bool ok, const juce::String& what)
{
    std::printf ("  [%s] %s\n", ok ? "pass" : "FAIL", what.toRawUTF8());
    if (! ok)
        ++failures;
}

juce::AudioBuffer<float> readWav (const juce::File& f)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
    if (r == nullptr)
    {
        std::printf ("cannot read %s\n", f.getFullPathName().toRawUTF8());
        std::exit (2);
    }
    juce::AudioBuffer<float> b ((int) r->numChannels, (int) r->lengthInSamples);
    r->read (&b, 0, b.getNumSamples(), 0, true, true);
    return b;
}

wrm1::Settings settingsFrom (const juce::var& v)
{
    wrm1::Settings s;
    s.inputDb = (float) (double) v["input"];
    s.warmth = (float) (double) v["warmth"];
    s.tone = (float) (double) v["tone"];
    s.body = (float) (double) v["body"];
    s.character = (int) v["character"];
    s.lowProtect = (float) (double) v["lowProtect"];
    s.mix = (float) (double) v["mix"];
    s.autoGain = (bool) v["autoGain"];
    s.outputDb = (float) (double) v["output"];
    s.oversampling = (int) v["oversampling"];
    return s;
}

/** Runs `input` through a freshly prepared engine in blocks of `blockSize`. */
juce::AudioBuffer<float> render (const juce::AudioBuffer<float>& input, const wrm1::Settings& s, double fs,
                                 int blockSize, int* latencyOut = nullptr)
{
    wrm1::Engine engine;
    engine.prepare (fs, blockSize, input.getNumChannels());
    engine.setSettings (s);
    engine.snapToTargets();

    juce::AudioBuffer<float> out (input);
    for (int start = 0; start < out.getNumSamples(); start += blockSize)
    {
        const int n = std::min (blockSize, out.getNumSamples() - start);
        juce::AudioBuffer<float> view (out.getArrayOfWritePointers(), out.getNumChannels(), start, n);
        engine.process (view);
    }
    if (latencyOut != nullptr)
        *latencyOut = engine.getLatencySamples();
    return out;
}

/** 20*log10(rms(a[n+offset] - b[n]) / rms(b[n])) over the overlapping range. */
double residualDb (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b, int offset)
{
    double err = 0.0, ref = 0.0;
    const int n = b.getNumSamples() - offset;
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < n; ++i)
        {
            const double d = (double) a.getSample (c, i + offset) - b.getSample (c, i);
            err += d * d;
            ref += (double) b.getSample (c, i) * b.getSample (c, i);
        }
    return 10.0 * std::log10 ((err + 1e-30) / (ref + 1e-30));
}
} // namespace

int main (int argc, char** argv)
{
    const juce::File dir (argc > 1 ? juce::String (argv[1]) : juce::String (RAINR_GOLDEN_DIR));
    const auto manifest = juce::JSON::parse (dir.getChildFile ("manifest.json"));
    const double fs = (double) manifest["sampleRate"];
    const auto input = readWav (dir.getChildFile (manifest["input"].toString()));

    std::printf ("WRM1 engine vs Python reference (%s)\n", dir.getFullPathName().toRawUTF8());
    for (const auto& c : *manifest["cases"].getArray())
    {
        int latency = 0;
        const auto s = settingsFrom (c["settings"]);
        const auto out = render (input, s, fs, 333, &latency); // odd block size on purpose
        const auto expected = readWav (dir.getChildFile (c["file"].toString()));
        const double res = residualDb (out, expected, latency);
        if (const auto dump = juce::SystemStats::getEnvironmentVariable ("WRM1_DUMP_DIR", {}); dump.isNotEmpty())
        {
            // Debug aid: write the engine output for inspection in Python.
            juce::WavAudioFormat wav;
            const juce::File f = juce::File (dump).getChildFile (c["file"].toString());
            f.deleteFile();
            std::unique_ptr<juce::OutputStream> stream (f.createOutputStream());
            const auto options = juce::AudioFormatWriterOptions {}
                                     .withSampleRate (fs)
                                     .withNumChannels (out.getNumChannels())
                                     .withBitsPerSample (32)
                                     .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);
            if (stream != nullptr)
                if (auto w = wav.createWriterFor (stream, options))
                    w->writeFromAudioSampleBuffer (out, 0, out.getNumSamples());
        }
        const double limit = (double) c["maxResidualDb"];
        check (res <= limit, juce::String::formatted ("%-28s residual %7.1f dB (limit %5.1f), latency %d",
                                                       c["name"].toString().toRawUTF8(), res, limit, latency));
    }

    std::printf ("Engine behaviour\n");
    {
        wrm1::Settings s;
        s.oversampling = 0;
        int latency = -1;
        render (input, s, fs, 512, &latency);
        check (latency == 0, "zero latency with oversampling Off");
    }
    {
        wrm1::Settings s; // defaults, 2x
        const auto a = render (input, s, fs, 512);
        const auto b = render (input, s, fs, 37);
        check (residualDb (a, b, 0) < -120.0, "same output for block sizes 512 and 37");
    }
    {
        // Every control moving every block, including character and oversampling switches.
        wrm1::Engine engine;
        engine.prepare (fs, 256, 2);
        juce::Random rng (42);
        juce::AudioBuffer<float> buf (2, 256);
        float peak = 0.0f;
        bool finite = true;
        for (int block = 0; block < 400; ++block)
        {
            wrm1::Settings s;
            s.warmth = rng.nextFloat();
            s.inputDb = rng.nextFloat() * 24.0f - 12.0f;
            s.tone = rng.nextFloat() * 2.0f - 1.0f;
            s.body = rng.nextFloat();
            s.character = rng.nextInt (3);
            s.lowProtect = rng.nextFloat();
            s.mix = rng.nextFloat();
            s.autoGain = rng.nextBool();
            s.oversampling = block % 50 == 0 ? rng.nextInt (3) : s.oversampling;
            engine.setSettings (s);
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < 256; ++i)
                    buf.setSample (c, i, input.getSample (c, (block * 256 + i) % input.getNumSamples()));
            engine.process (buf);
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < 256; ++i)
                {
                    const float v = buf.getSample (c, i);
                    finite = finite && std::isfinite (v);
                    peak = std::max (peak, std::abs (v));
                }
        }
        check (finite, "output stays finite while every control moves");
        check (peak < 8.0f, juce::String::formatted ("output stays bounded while every control moves (peak %.2f)", peak));
    }
    {
        wrm1::Settings s;
        juce::AudioBuffer<float> mono (1, input.getNumSamples());
        mono.copyFrom (0, 0, input, 0, 0, input.getNumSamples());
        const auto out = render (mono, s, fs, 512);
        check (out.getNumChannels() == 1 && std::isfinite (out.getSample (0, 1000)), "mono processing");
    }

    std::printf ("CPU (information only)\n");
    for (int os = 0; os < 3; ++os)
    {
        // 10 s of stereo at 48 kHz in 256-sample blocks, default settings.
        wrm1::Settings s;
        s.oversampling = os;
        wrm1::Engine engine;
        engine.prepare (fs, 256, 2);
        engine.setSettings (s);
        engine.snapToTargets();
        juce::AudioBuffer<float> buf (2, 256);
        const int blocks = (int) (10.0 * fs / 256);
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        for (int b = 0; b < blocks; ++b)
        {
            for (int c = 0; c < 2; ++c)
                buf.copyFrom (c, 0, input, c, (b * 256) % (input.getNumSamples() - 256), 256);
            engine.process (buf);
        }
        const double ms = juce::Time::getMillisecondCounterHiRes() - t0;
        std::printf ("  oversampling %-3s %5.2f %% of one core in real time\n", os == 0 ? "Off" : os == 1 ? "2x" : "4x", ms / 10000.0 * 100.0);
    }

    std::printf (failures == 0 ? "ALL PASSED\n" : "%d FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}
