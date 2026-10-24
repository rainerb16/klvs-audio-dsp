// Renders the WRM1 editor to a PNG (2x scale) after playing a test signal through the plugin,
// so the meters and scope show real values. Development helper, not shipped.

#include "PluginProcessor.h"

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        std::printf ("usage: WRM1Snapshot out.png [param=value ...]\n");
        return 2;
    }

    juce::ScopedJuceInitialiser_GUI gui;
    wrm1::Processor processor;

    for (int i = 2; i < argc; ++i)
    {
        const juce::String arg (argv[i]);
        if (auto* p = processor.state.getParameter (arg.upToFirstOccurrenceOf ("=", false, false)))
            p->setValueNotifyingHost (p->convertTo0to1 (arg.fromFirstOccurrenceOf ("=", false, false).getFloatValue()));
        else
            std::printf ("unknown parameter: %s\n", arg.toRawUTF8());
    }

    processor.prepareToPlay (48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditorAndMakeActive());

    // Play a -12 dBFS mix-like test tone so the meters have something to show.
    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    double phase = 0.0;
    for (int block = 0; block < 40; ++block)
    {
        for (int i = 0; i < 512; ++i, phase += 1.0 / 48000.0)
        {
            const auto v = (float) (0.25 * std::sin (juce::MathConstants<double>::twoPi * 110.0 * phase)
                                    + 0.12 * std::sin (juce::MathConstants<double>::twoPi * 1700.0 * phase));
            buffer.setSample (0, i, v);
            buffer.setSample (1, i, v * 0.9f);
        }
        processor.processBlock (buffer, midi);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
    }

    const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);
    const juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
    out.deleteFile();
    juce::FileOutputStream stream (out);
    juce::PNGImageFormat png;
    if (! stream.openedOk() || ! png.writeImageToStream (image, stream))
    {
        std::printf ("could not write %s\n", out.getFullPathName().toRawUTF8());
        return 1;
    }
    std::printf ("wrote %s (%d x %d)\n", out.getFullPathName().toRawUTF8(), image.getWidth(), image.getHeight());
    editor.reset();
    return 0;
}
