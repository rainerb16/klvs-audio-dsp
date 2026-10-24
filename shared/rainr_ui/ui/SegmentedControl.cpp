namespace rainr::ui
{
SegmentedControl::SegmentedControl (juce::RangedAudioParameter& parameter, const juce::StringArray& labels)
    : attachment (parameter, [this] (float value) { select (juce::roundToInt (value)); })
{
    for (int i = 0; i < labels.size(); ++i)
    {
        auto* s = segments.add (new Segment (labels[i]));
        s->setClickingTogglesState (false);
        s->setTitle (labels[i]);
        s->onClick = [this, i] { attachment.setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (s);
    }
    setTitle (parameter.getName (64));
    attachment.sendInitialUpdate();
}

void SegmentedControl::select (int index)
{
    selected = index;
    for (int i = 0; i < segments.size(); ++i)
        segments[i]->setToggleState (i == index, juce::dontSendNotification);
    repaint();
}

void SegmentedControl::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    g.setColour (colours::well);
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawHorizontalLine (getHeight() - 1, 7.0f, r.getRight() - 7.0f);
}

void SegmentedControl::resized()
{
    auto r = getLocalBounds().reduced (3);
    const int w = segments.isEmpty() ? 0 : (r.getWidth() - 2 * (segments.size() - 1)) / segments.size();
    for (auto* s : segments)
    {
        s->setBounds (r.removeFromLeft (w));
        r.removeFromLeft (2);
    }
}

void SegmentedControl::Segment::paintButton (juce::Graphics& g, bool highlighted, bool)
{
    const auto r = getLocalBounds().toFloat();
    if (getToggleState())
    {
        juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 3, { 0, 1 }).drawForRectangle (g, getLocalBounds());
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a40), 0.0f, 0.0f, juce::Colour (0xff222226), 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.drawHorizontalLine (1, 5.0f, r.getRight() - 5.0f);
    }
    g.setColour (getToggleState() ? colours::text : (highlighted ? colours::grey : colours::label));
    g.setFont (Fonts::sans (12.0f));
    g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred);
}
} // namespace rainr::ui
