namespace rainr::ui
{
namespace
{
    constexpr float tickDb[] = { -48.0f, -24.0f, -12.0f, -6.0f, 0.0f };
    constexpr float fallDbPerSecond = 24.0f;
    constexpr float segment = 5.0f, segmentGap = 1.0f;
} // namespace

float LevelMeter::positionFor (float dB) noexcept
{
    if (dB <= tickDb[0])
        return 0.0f;
    for (int i = 0; i < 4; ++i)
        if (dB <= tickDb[i + 1])
            return ((float) i + (dB - tickDb[i]) / (tickDb[i + 1] - tickDb[i])) / 4.0f;
    return 1.0f;
}

void LevelMeter::setPeaks (float left, float right, double secondsSinceLast)
{
    bool changed = false;
    const std::array<float, 2> peaks { left, right };
    for (size_t c = 0; c < 2; ++c)
    {
        const float newDb = juce::Decibels::gainToDecibels (peaks[c], -100.0f);
        const float fallen = displayDb[c] - fallDbPerSecond * (float) secondsSinceLast;
        const float next = std::max (newDb, fallen);
        changed = changed || std::abs (next - displayDb[c]) > 0.05f;
        displayDb[c] = std::max (next, -100.0f);
    }
    if (changed)
        repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    const auto h = (float) getHeight();
    for (size_t c = 0; c < 2; ++c)
    {
        const auto slot = juce::Rectangle<float> ((float) c * (barWidth + barGap), 0.0f, (float) barWidth, h);
        g.setColour (colours::slot);
        g.fillRoundedRectangle (slot, 3.0f);

        const float lit = positionFor (displayDb[c]) * h;
        for (float y = 0.0f; y + segment <= lit + 0.5f; y += segment)
        {
            const float frac = (y + segment * 0.5f) / h;
            juce::Colour col = frac < 0.55f ? juce::Colour (0xff6e6e74).interpolatedWith (colours::grey, frac / 0.55f)
                             : frac < 0.85f ? colours::grey.interpolatedWith (colours::text, (frac - 0.55f) / 0.30f)
                             : frac < 0.92f ? colours::text
                                            : colours::red;
            g.setColour (col);
            g.fillRect (slot.getX() + 1.0f, h - y - (segment - segmentGap), (float) barWidth - 2.0f, segment - segmentGap);
        }
    }
}
} // namespace rainr::ui
