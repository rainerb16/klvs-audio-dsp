namespace rainr::ui
{
namespace
{
    juce::Typeface::Ptr load (const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    }

    struct Typefaces
    {
        juce::Typeface::Ptr sans = load (RainrUiFonts::IBMPlexSansVariable_ttf, RainrUiFonts::IBMPlexSansVariable_ttfSize);
        juce::Typeface::Ptr mono = load (RainrUiFonts::IBMPlexMonoRegular_ttf, RainrUiFonts::IBMPlexMonoRegular_ttfSize);
        juce::Typeface::Ptr monoMedium = load (RainrUiFonts::IBMPlexMonoMedium_ttf, RainrUiFonts::IBMPlexMonoMedium_ttfSize);
    };

    const Typefaces& typefaces()
    {
        static const Typefaces t;
        return t;
    }
} // namespace

juce::Font Fonts::sans (float height)
{
    return juce::Font (juce::FontOptions {}.withTypeface (typefaces().sans).withHeight (height));
}

juce::Font Fonts::mono (float height, bool medium, float tracking)
{
    const auto& t = typefaces();
    return juce::Font (juce::FontOptions {}.withTypeface (medium ? t.monoMedium : t.mono).withHeight (height).withKerningFactor (tracking));
}
} // namespace rainr::ui
