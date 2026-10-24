namespace rainr::ui
{
void TransferScope::setData (const Data& d)
{
    data = d;
    repaint();
}

void TransferScope::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (colours::well);
    g.fillRoundedRectangle (area, 8.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (area.reduced (0.5f), 8.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawHorizontalLine (getHeight() - 1, 8.0f, area.getRight() - 8.0f);

    area = area.reduced (10.0f);
    auto footer = area.removeFromBottom (14.0f);
    area.removeFromBottom (8.0f);
    auto labels = area.removeFromBottom (12.0f);
    area.removeFromBottom (8.0f);
    auto bars = area.removeFromBottom (36.0f);
    area.removeFromBottom (8.0f);
    const auto plot = area.reduced (6.0f);

    // Grid and the straight "no effect" line.
    g.setColour (juce::Colour (0xff1f1f23));
    for (int i = 0; i <= 8; ++i)
    {
        const float fx = plot.getX() + plot.getWidth() * (float) i / 8.0f;
        const float fy = plot.getY() + plot.getHeight() * (float) i / 8.0f;
        g.drawVerticalLine (juce::roundToInt (fx), plot.getY(), plot.getBottom());
        g.drawHorizontalLine (juce::roundToInt (fy), plot.getX(), plot.getRight());
    }
    juce::Path diagonal;
    diagonal.startNewSubPath (plot.getBottomLeft());
    diagonal.lineTo (plot.getTopRight());
    juce::Path dashed;
    const float dashes[] = { 4.0f, 5.0f };
    juce::PathStrokeType (1.0f).createDashedStroke (dashed, diagonal, dashes, 2);
    g.setColour (juce::Colour (0xff4a4a50));
    g.fillPath (dashed);

    // Curve
    juce::Path curve;
    for (int i = 0; i < curvePoints; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * (float) i / (curvePoints - 1);
        const float y = plot.getBottom() - (std::clamp (data.curve[(size_t) i], -1.0f, 1.0f) + 1.0f) * 0.5f * plot.getHeight();
        i == 0 ? curve.startNewSubPath (x, y) : curve.lineTo (x, y);
    }
    const auto stroke = [&] (float w, juce::Colour c) {
        g.setColour (c);
        g.strokePath (curve, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };
    stroke (8.0f, colours::red.withAlpha (0.12f));
    stroke (5.0f, colours::red.withAlpha (0.25f));
    stroke (2.2f, colours::red);

    g.setColour (colours::faint);
    g.setFont (Fonts::mono (10.0f));
    g.drawText (juce::String (juce::CharPointer_UTF8 ("IN \xe2\x86\x92")), plot.withTrimmedTop (plot.getHeight() - 14.0f).withTrimmedLeft (plot.getWidth() - 44.0f), juce::Justification::centredRight);
    g.drawText ("OUT", plot.withHeight (14.0f).withWidth (30.0f).translated (4.0f, 4.0f), juce::Justification::centredLeft);

    // Harmonic bars, H2 highlighted.
    const float colW = (bars.getWidth() - 4.0f * 6.0f) / 5.0f;
    for (int i = 0; i < 5; ++i)
    {
        const float x = bars.getX() + (float) i * (colW + 6.0f);
        const float hgt = std::clamp ((data.harmonicsDb[(size_t) i] + 80.0f) / 80.0f * bars.getHeight(), 3.0f, bars.getHeight());
        const auto bar = juce::Rectangle<float> (x, bars.getBottom() - hgt, colW, hgt);
        if (i == 1)
        {
            g.setColour (colours::red.withAlpha (0.25f));
            g.fillRoundedRectangle (bar.expanded (2.0f), 2.0f);
        }
        g.setColour (i == 1 ? colours::red : colours::grey);
        g.fillRoundedRectangle (bar, 1.0f);
        g.setColour (colours::muted);
        g.drawText ("H" + juce::String (i + 1), juce::Rectangle<float> (x, labels.getY(), colW, labels.getHeight()), juce::Justification::centred);
    }

    // Readouts: "label value" on the left, "label value" right-aligned on the right.
    const auto labelFont = Fonts::mono (11.0f), valueFont = Fonts::mono (11.0f, true);
    const auto widthOf = [] (const juce::Font& f, const juce::String& t) {
        juce::GlyphArrangement ga;
        ga.addLineOfText (f, t, 0.0f, 0.0f);
        return ga.getBoundingBox (0, -1, true).getWidth() + 1.0f;
    };
    auto left = footer.removeFromLeft (footer.getWidth() * 0.5f);
    g.setFont (labelFont);
    g.setColour (colours::muted);
    g.drawText (data.leftLabel, left.removeFromLeft (widthOf (labelFont, data.leftLabel + " ")), juce::Justification::centredLeft);
    g.setFont (valueFont);
    g.setColour (colours::text);
    g.drawText (data.leftValue, left, juce::Justification::centredLeft);
    g.drawText (data.rightValue, footer.removeFromRight (widthOf (valueFont, data.rightValue)), juce::Justification::centredRight);
    g.setFont (labelFont);
    g.setColour (colours::muted);
    g.drawText (data.rightLabel + " ", footer, juce::Justification::centredRight);
}
} // namespace rainr::ui
