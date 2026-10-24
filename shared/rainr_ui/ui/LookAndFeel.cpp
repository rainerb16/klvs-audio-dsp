namespace rainr::ui
{
namespace
{
    constexpr float startAngle = juce::MathConstants<float>::pi * 1.25f;
    constexpr float endAngle = juce::MathConstants<float>::pi * 2.75f;

    juce::Path arc (juce::Point<float> c, float r, float from, float to)
    {
        juce::Path p;
        p.addCentredArc (c.x, c.y, r, r, 0.0f, from, to, true);
        return p;
    }

    void strokeArc (juce::Graphics& g, const juce::Path& p, float width)
    {
        g.strokePath (p, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
} // namespace

LookAndFeel::LookAndFeel()
{
    using namespace colours;
    setColourScheme ({ background, panel, background, border, text, red, text, red, text });
    setColour (juce::ResizableWindow::backgroundColourId, background);
    setColour (juce::Label::textColourId, text);
}

void configureKnob (juce::Slider& s, bool big, bool bipolar)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (startAngle, endAngle, true);
    s.setMouseDragSensitivity (big ? 260 : 200);
    s.setVelocityBasedMode (false);
    s.getProperties().set (LookAndFeel::bigKnob, big);
    s.getProperties().set (LookAndFeel::bipolarKnob, bipolar);
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                    float rotaryStart, float rotaryEnd, juce::Slider& slider)
{
    // Proportions follow the mockup's SVG knobs (radius R = half the knob's size).
    const bool big = slider.getProperties()[bigKnob];
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const float R = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = rotaryStart + sliderPos * (rotaryEnd - rotaryStart);

    const float ringR = R * (big ? 0.924f : 0.885f);
    const float ringW = R * (big ? 0.086f : 0.115f);
    const float bezelR = R * (big ? 0.762f : 0.654f);
    const float capR = R * (big ? 0.648f : 0.538f);
    const auto accent = big ? colours::red : colours::grey;

    // Value ring: dark track, then the value (with a glow on the main knob).
    g.setColour (colours::slot);
    strokeArc (g, arc (c, ringR, rotaryStart, rotaryEnd), ringW);
    const bool bipolar = slider.getProperties()[bipolarKnob];
    const float from = bipolar ? (rotaryStart + rotaryEnd) * 0.5f : rotaryStart;
    if (std::abs (angle - from) > 0.01f)
    {
        const auto value = arc (c, ringR, std::min (from, angle), std::max (from, angle));
        if (big)
        {
            g.setColour (accent.withAlpha (0.18f));
            strokeArc (g, value, ringW * 2.6f);
            g.setColour (accent.withAlpha (0.3f));
            strokeArc (g, value, ringW * 1.7f);
        }
        g.setColour (accent);
        strokeArc (g, value, ringW * (big ? 0.8f : 1.0f));
    }

    // Body: drop shadow, metal bezel, glossy cap, specular highlight.
    juce::Path bezel;
    bezel.addEllipse (juce::Rectangle<float> (bezelR * 2.0f, bezelR * 2.0f).withCentre (c));
    juce::DropShadow (juce::Colours::black.withAlpha (0.65f), juce::roundToInt (R * 0.12f), { 0, juce::roundToInt (R * 0.07f) })
        .drawForPath (g, bezel);

    juce::ColourGradient bezelFill (juce::Colour (0xff5a5a61), c.x, c.y - bezelR, juce::Colour (0xff0c0c0e), c.x, c.y + bezelR, false);
    bezelFill.addColour (0.45, juce::Colour (0xff2a2a2e));
    g.setGradientFill (bezelFill);
    g.fillPath (bezel);
    if (big)
    {
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.strokePath (bezel, juce::PathStrokeType (1.0f));
    }

    const auto capArea = juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (c);
    const auto hot = juce::Point<float> (c.x - capR * 0.24f, c.y - capR * 0.4f);
    juce::ColourGradient capFill (juce::Colour (0xff46464d), hot, juce::Colour (0xff101012), hot.translated (capR * 1.6f, 0.0f), true);
    capFill.addColour (0.45, juce::Colour (0xff232327));
    g.setGradientFill (capFill);
    g.fillEllipse (capArea);
    if (big)
    {
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (capArea, 1.5f);
    }

    const auto spec = juce::Rectangle<float> (R * (big ? 0.84f : 0.69f), R * (big ? 0.42f : 0.35f))
                          .withCentre ({ c.x, c.y - R * (big ? 0.352f : 0.269f) });
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.22f), spec.getCentreX(), spec.getY(),
                                             juce::Colours::white.withAlpha (0.0f), spec.getCentreX(), spec.getBottom(), false));
    g.fillEllipse (spec);

    // Pointer.
    const auto tip = c.getPointOnCircumference (R * (big ? 0.562f : 0.5f), angle);
    const auto tail = c.getPointOnCircumference (R * (big ? 0.257f : 0.308f), angle);
    g.setColour (colours::text);
    g.drawLine ({ tail, tip }, R * (big ? 0.048f : 0.077f));
    if (big)
    {
        g.setColour (colours::red);
        g.fillEllipse (juce::Rectangle<float> (R * 0.066f, R * 0.066f).withCentre (tip));
    }

    if (slider.hasKeyboardFocus (true))
    {
        g.setColour (colours::grey.withAlpha (0.35f));
        g.drawEllipse (juce::Rectangle<float> (ringR * 2.0f + ringW * 2.0f, ringR * 2.0f + ringW * 2.0f).withCentre (c), 1.0f);
    }
}

void LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const float radius = r.getHeight() * 0.5f;

    juce::Path pill;
    pill.addRoundedRectangle (r, radius);
    juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 4, { 0, 2 }).drawForPath (g, pill);
    g.setGradientFill (juce::ColourGradient (juce::Colour (highlighted ? 0xff34343a : 0xff2c2c31), 0.0f, r.getY(),
                                             juce::Colour (0xff18181b), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, radius);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r, radius, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.1f));
    g.drawHorizontalLine (juce::roundToInt (r.getY() + 1.0f), r.getX() + radius, r.getRight() - radius);

    // LED
    const auto led = juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ r.getX() + 15.0f, r.getCentreY() });
    if (b.getToggleState())
    {
        g.setColour (colours::red.withAlpha (0.35f));
        g.fillEllipse (led.expanded (3.0f));
        g.setColour (colours::red);
    }
    else
    {
        g.setColour (juce::Colour (0xff3a3a3f));
    }
    g.fillEllipse (led);

    g.setColour (b.getToggleState() ? colours::text : colours::label);
    g.setFont (Fonts::mono (10.0f, false, 0.12f));
    g.drawText (b.getButtonText().toUpperCase(), r.withTrimmedLeft (26.0f).withTrimmedRight (10.0f), juce::Justification::centredLeft);
}

void LookAndFeel::drawRecess (juce::Graphics& g, juce::Rectangle<float> area, float cornerRadius)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0b0b0c), 0.0f, area.getY(),
                                             juce::Colour (0xff121214), 0.0f, area.getBottom(), false));
    g.fillRoundedRectangle (area, cornerRadius);

    // Inner shadow along the top edge.
    juce::Graphics::ScopedSaveState save (g);
    juce::Path clip;
    clip.addRoundedRectangle (area, cornerRadius);
    g.reduceClipRegion (clip);
    for (int i = 0; i < 6; ++i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.35f * (1.0f - (float) i / 6.0f)));
        g.drawRoundedRectangle (area.reduced ((float) i * 0.5f).translated (0.0f, (float) i * 0.6f), cornerRadius, 1.0f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawHorizontalLine (juce::roundToInt (area.getBottom() - 1.0f), area.getX() + cornerRadius, area.getRight() - cornerRadius);
}

void LookAndFeel::drawCapsLabel (juce::Graphics& g, const juce::String& textToDraw, juce::Rectangle<float> area,
                                 juce::Justification just, float height, juce::Colour colour)
{
    g.setColour (colour);
    g.setFont (Fonts::mono (height, false, 0.2f));
    g.drawText (textToDraw, area, just, false);
}
} // namespace rainr::ui
