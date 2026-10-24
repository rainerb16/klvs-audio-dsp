#include "PluginEditor.h"
#include "Parameters.h"

#include <WRM1Assets.h>

namespace wrm1
{
namespace
{
    namespace colours = rainr::ui::colours;
    using rainr::ui::LookAndFeel;

    constexpr int width = 980, height = 506, headerHeight = 68;

    // Layout (pixels), from the mockup: side columns, scope, centre, bottom tray.
    const juce::Rectangle<int> inColumn { 20, 86, 120, 280 };
    const juce::Rectangle<int> scopeArea { 160, 86, 300, 260 };
    const juce::Rectangle<int> centre { 480, 86, 340, 270 };
    const juce::Rectangle<int> outColumn { 840, 86, 120, 280 };
    const juce::Rectangle<int> tray { 16, 368, 948, 120 };
    constexpr int meterTop = 110, meterHeight = 116;

    juce::RangedAudioParameter& parameter (juce::AudioProcessorValueTreeState& s, const char* id)
    {
        auto* p = s.getParameter (id);
        jassert (p != nullptr);
        return *p;
    }

    /** Meter bars and their scale, centred in a side column; scale on the outer side. */
    juce::Rectangle<int> meterBounds (juce::Rectangle<int> column)
    {
        const int barsWidth = 2 * rainr::ui::LevelMeter::barWidth + rainr::ui::LevelMeter::barGap;
        const int groupWidth = barsWidth + 6 + 22;
        const int x = column.getX() + (column.getWidth() - groupWidth) / 2;
        const bool scaleOnLeft = column.getX() < width / 2;
        return { scaleOnLeft ? x + 28 : x, meterTop, barsWidth, meterHeight };
    }

    void drawMeterScale (juce::Graphics& g, juce::Rectangle<int> meter, bool left)
    {
        g.setFont (rainr::ui::Fonts::mono (10.0f));
        g.setColour (colours::muted);
        for (float dB : { 0.0f, -6.0f, -12.0f, -24.0f, -48.0f })
        {
            const float y = (float) meter.getBottom() - rainr::ui::LevelMeter::positionFor (dB) * (float) meter.getHeight();
            const auto r = left ? juce::Rectangle<float> ((float) meter.getX() - 28.0f, y - 6.0f, 22.0f, 12.0f)
                                : juce::Rectangle<float> ((float) meter.getRight() + 6.0f, y - 6.0f, 22.0f, 12.0f);
            g.drawText (juce::String ((int) dB), r, left ? juce::Justification::centredRight : juce::Justification::centredLeft);
        }
    }
} // namespace

//==============================================================================
KnobView::KnobView (juce::AudioProcessorValueTreeState& state, const char* paramId, const juce::String& knobName, Style style)
    : param (parameter (state, paramId)), name (knobName), big (style == Style::big), attachment (state, paramId, slider)
{
    rainr::ui::configureKnob (slider, big, style == Style::smallBipolar);
    slider.setTitle (name);
    slider.setDoubleClickReturnValue (true, param.convertFrom0to1 (param.getDefaultValue()));
    slider.onValueChange = [this] { repaint(); };
    addAndMakeVisible (slider);
}

void KnobView::resized()
{
    const int size = big ? getWidth() : knobSize;
    slider.setBounds ((getWidth() - size) / 2, 0, size, size);
}

void KnobView::paint (juce::Graphics& g)
{
    const auto value = param.getCurrentValueAsText();
    if (big)
    {
        auto r = getLocalBounds().withTrimmedTop (getWidth()).toFloat();
        LookAndFeel::drawCapsLabel (g, name.toUpperCase(), r.removeFromTop (16.0f), juce::Justification::centred, 12.0f, colours::text);
        g.setFont (rainr::ui::Fonts::mono (15.0f));
        g.setColour (colours::red);
        g.drawText (value, r.removeFromTop (20.0f), juce::Justification::centred);
        return;
    }
    auto r = getLocalBounds().withTrimmedTop (knobSize + 4).toFloat();
    g.setFont (rainr::ui::Fonts::sans (12.0f));
    g.setColour (colours::grey);
    g.drawText (name, r.removeFromTop (15.0f), juce::Justification::centred);
    g.setFont (rainr::ui::Fonts::mono (12.0f));
    g.setColour (colours::text);
    g.drawText (value, r.removeFromTop (15.0f), juce::Justification::centred);
}

//==============================================================================
Editor::Editor (Processor& p)
    : AudioProcessorEditor (p),
      pluginProcessor (p),
      lockup (juce::ImageCache::getFromMemory (WRM1Assets::lockup_png, WRM1Assets::lockup_pngSize)),
      input (p.state, params::input, "Input", KnobView::Style::smallBipolar),
      warmth (p.state, params::warmth, "Warmth", KnobView::Style::big),
      output (p.state, params::output, "Output", KnobView::Style::smallBipolar),
      tone (p.state, params::tone, "Tone", KnobView::Style::smallBipolar),
      body (p.state, params::body, "Body", KnobView::Style::small),
      lowProtect (p.state, params::lowProtect, "Low Protect", KnobView::Style::small),
      mix (p.state, params::mix, "Mix", KnobView::Style::small),
      character (parameter (p.state, params::character), params::characterChoices),
      oversampling (parameter (p.state, params::oversampling), params::oversamplingChoices),
      autoGainAttachment (p.state, params::autoGain, autoGain)
{
    setLookAndFeel (&lookAndFeel);
    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &input, &warmth, &output, &tone, &body, &lowProtect, &mix, &character, &oversampling, &autoGain,
             &inputMeter, &outputMeter, &scope })
        addAndMakeVisible (c);

    setSize (width, height);
    updateScope();
    startTimerHz (30);
}

Editor::~Editor()
{
    setLookAndFeel (nullptr);
}

void Editor::resized()
{
    const auto knobIn = [] (juce::Rectangle<int> column, int y) {
        return juce::Rectangle<int> (column.getCentreX() - KnobView::smallWidth / 2, y, KnobView::smallWidth, KnobView::smallHeight);
    };

    inputMeter.setBounds (meterBounds (inColumn));
    input.setBounds (knobIn (inColumn, meterTop + meterHeight + 8));
    outputMeter.setBounds (meterBounds (outColumn));
    output.setBounds (knobIn (outColumn, meterTop + meterHeight + 8));
    autoGain.setBounds (outColumn.getCentreX() - 56, output.getBottom() + 10, 112, 28);

    scope.setBounds (scopeArea);

    character.setBounds (centre.getCentreX() - 111, centre.getY(), 222, 36);
    warmth.setBounds (centre.getCentreX() - 90, centre.getY() + 46, 180, 180 + 36);

    int x = tray.getX() + 16;
    for (auto* k : { &tone, &body, &lowProtect, &mix })
    {
        k->setBounds (x, tray.getY() + 16, KnobView::smallWidth, KnobView::smallHeight);
        x += KnobView::smallWidth + 22;
    }
    oversampling.setBounds (tray.getRight() - 16 - 210, tray.getCentreY() - 6, 210, 36);
}

void Editor::paint (juce::Graphics& g)
{
    const auto all = getLocalBounds().toFloat();

    // Faceplate
    juce::ColourGradient face (juce::Colour (0xff1a1a1d), 0.0f, 0.0f, colours::background, 0.0f, all.getBottom(), false);
    face.addColour (0.45, juce::Colour (0xff111113));
    g.setGradientFill (face);
    g.fillAll();

    // Header
    const auto header = all.withHeight ((float) headerHeight);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1e1e21), 0.0f, 0.0f, juce::Colour (0xff121214), 0.0f, header.getBottom(), false));
    g.fillRect (header);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (0, 0.0f, all.getRight());
    g.setColour (juce::Colours::black);
    g.drawHorizontalLine (headerHeight - 1, 0.0f, all.getRight());
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.drawHorizontalLine (headerHeight, 0.0f, all.getRight());

    if (lockup.isValid())
    {
        g.setOpacity (1.0f); // images take the current opacity; the last colour was a faint line
        const float h = 40.0f, w = h * (float) lockup.getWidth() / (float) lockup.getHeight();
        g.drawImage (lockup, { 24.0f, 14.0f, w, h }, juce::RectanglePlacement::centred);
    }
    const auto right = header.withTrimmedRight (24.0f).withTrimmedLeft (all.getWidth() - 260.0f);
    LookAndFeel::drawCapsLabel (g, "MIX BUS WARMTH", right.withY (18.0f).withHeight (14.0f), juce::Justification::centredRight, 11.0f, colours::grey);
    LookAndFeel::drawCapsLabel (g, "v" RAINR_PLUGIN_VERSION, right.withY (36.0f).withHeight (14.0f), juce::Justification::centredRight, 11.0f, colours::muted);

    // Meter labels and scales
    LookAndFeel::drawCapsLabel (g, "IN", inColumn.withHeight (14).toFloat(), juce::Justification::centred);
    LookAndFeel::drawCapsLabel (g, "OUT", outColumn.withHeight (14).toFloat(), juce::Justification::centred);
    drawMeterScale (g, inputMeter.getBounds(), true);
    drawMeterScale (g, outputMeter.getBounds(), false);

    // Bottom tray
    LookAndFeel::drawRecess (g, tray.toFloat(), 10.0f);
    LookAndFeel::drawCapsLabel (g, "OVERSAMPLING", oversampling.getBounds().toFloat().translated (0.0f, -20.0f).withHeight (14.0f),
                                juce::Justification::centredRight, 10.0f);
}

void Editor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double dt = lastTimerMs > 0.0 ? (now - lastTimerMs) / 1000.0 : 1.0 / 30.0;
    lastTimerMs = now;

    const auto in = pluginProcessor.takeInputPeaks();
    const auto out = pluginProcessor.takeOutputPeaks();
    inputMeter.setPeaks (in[0], in[1], dt);
    outputMeter.setPeaks (out[0], out[1], dt);

    updateScope();
}

void Editor::updateScope()
{
    auto& s = pluginProcessor.state;
    const auto get = [&] (const char* id) { return s.getRawParameterValue (id)->load(); };
    const std::array<float, 7> key { get (params::character), get (params::warmth), get (params::input),
                                     get (params::lowProtect), get (params::body), get (params::tone), get (params::autoGain) };
    const bool unchanged = std::equal (key.begin(), key.end(), lastScopeKey.begin(),
                                       [] (float a, float b) { return juce::exactlyEqual (a, b); });
    if (unchanged && lastTimerMs > 0.0)
        return;
    lastScopeKey = key;

    const int ch = juce::roundToInt (key[0]);
    const double driveDb = driveDbFor (key[1], key[2]);

    rainr::ui::TransferScope::Data d;
    for (int i = 0; i < rainr::ui::TransferScope::curvePoints; ++i)
    {
        const double x = -1.0 + 2.0 * i / (rainr::ui::TransferScope::curvePoints - 1);
        d.curve[(size_t) i] = (float) transferCurve (ch, driveDb, x);
    }

    // Harmonics of a -6 dBFS sine through the curve (one-cycle DFT).
    constexpr int n = 512;
    std::array<double, 5> mags {};
    for (int k = 1; k <= 5; ++k)
    {
        double re = 0.0, im = 0.0;
        for (int i = 0; i < n; ++i)
        {
            const double ph = juce::MathConstants<double>::twoPi * i / n;
            const double y = transferCurve (ch, driveDb, 0.5 * std::sin (ph));
            re += y * std::cos (k * ph);
            im += y * std::sin (k * ph);
        }
        mags[(size_t) k - 1] = std::hypot (re, im);
    }
    for (size_t k = 0; k < 5; ++k)
        d.harmonicsDb[k] = (float) juce::Decibels::gainToDecibels (mags[k] / mags[0], -120.0);

    const auto dbText = [] (double v) {
        return juce::String (v < 0.0 ? "-" : "+") + juce::String (std::abs (v), 1) + " dB"; // same minus as the knobs
    };
    d.leftLabel = "2nd harmonic";
    d.leftValue = "-" + juce::String (juce::roundToInt (std::abs (d.harmonicsDb[1]))) + " dB";
    d.rightLabel = "Auto Gain";
    d.rightValue = key[6] >= 0.5f ? dbText (autoGainDb (ch, driveDb, key[3], key[4], key[5])) : juce::String ("off");
    scope.setData (d);
}
} // namespace wrm1
