// WRM1 processor tests: what the host sees (typed parameter values, saved state, bypass).
// Called from main() in WRM1EngineTests.cpp.

#include "WRM1ProcessorTests.h"
#include "Parameters.h"
#include "PluginProcessor.h"

int runProcessorTests (void (*check) (bool, const juce::String&))
{
    juce::ScopedJuceInitialiser_GUI gui; // the processor uses a timer for latency reporting
    std::printf ("Processor\n");

    const auto plain = [] (wrm1::Processor& p, const char* id) {
        auto* param = p.state.getParameter (id);
        return param->convertFrom0to1 (param->getValue());
    };
    const auto set = [] (wrm1::Processor& p, const char* id, float plainValue) {
        auto* param = p.state.getParameter (id);
        param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
    };

    {
        // Hosts that accept typed values parse the same text the plugin displays.
        wrm1::Processor p;
        auto* warmth = p.state.getParameter (wrm1::params::warmth);
        auto* tone = p.state.getParameter (wrm1::params::tone);
        auto* output = p.state.getParameter (wrm1::params::output);
        const auto near = [] (float a, float b) { return std::abs (a - b) < 1e-4f; };
        check (near (warmth->convertFrom0to1 (warmth->getValueForText ("50 %")), 0.5f), "typed \"50 %\" sets Warmth to 50 %");
        check (near (tone->convertFrom0to1 (tone->getValueForText ("Dark 30")), -0.3f)
                   && near (tone->convertFrom0to1 (tone->getValueForText ("Bright 30")), 0.3f),
               "typed \"Dark 30\" / \"Bright 30\" set Tone correctly");
        check (near (output->convertFrom0to1 (output->getValueForText ("-3.5 dB")), -3.5f), "typed \"-3.5 dB\" sets Output");
        check (warmth->getText (warmth->getValueForText (warmth->getText (0.37f, 64)), 64) == warmth->getText (0.37f, 64),
               "parameter text round-trips");
    }
    {
        // Saved state round-trips, and a parameter missing from an older project loads at its
        // default rather than keeping whatever the knob was at.
        wrm1::Processor saver;
        set (saver, wrm1::params::warmth, 0.7f);
        set (saver, wrm1::params::character, 2.0f);
        juce::MemoryBlock data;
        saver.getStateInformation (data);

        auto xml = juce::AudioProcessor::getXmlFromBinary (data.getData(), (int) data.getSize());
        for (auto* child : xml->getChildIterator())
            if (child->getStringAttribute ("id") == wrm1::params::body)
            {
                xml->removeChildElement (child, true);
                break;
            }
        juce::MemoryBlock older;
        juce::AudioProcessor::copyXmlToBinary (*xml, older);

        wrm1::Processor loader;
        set (loader, wrm1::params::body, 0.9f);
        loader.setStateInformation (older.getData(), (int) older.getSize());
        const auto near = [] (float a, float b) { return std::abs (a - b) < 1e-4f; };
        check (near (plain (loader, wrm1::params::warmth), 0.7f) && near (plain (loader, wrm1::params::character), 2.0f),
               "saved state round-trips");
        check (near (plain (loader, wrm1::params::body), 0.3f), "a parameter missing from a saved project loads at its default");

        loader.setStateInformation ("garbage", 7); // foreign data is ignored, nothing crashes
        check (near (plain (loader, wrm1::params::warmth), 0.7f), "unreadable state data is ignored");
    }
    {
        // Host bypass keeps the reported latency.
        wrm1::Processor p;
        p.prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> buf (2, 512);
        juce::MidiBuffer midi;
        buf.clear();
        buf.setSample (0, 5, 1.0f);
        p.processBlockBypassed (buf, midi);
        const int latency = p.getLatencySamples();
        check (latency > 0 && juce::exactlyEqual (buf.getSample (0, 5 + latency), 1.0f),
               juce::String::formatted ("host bypass keeps the reported latency (%d samples)", latency));
    }
    return 0;
}
