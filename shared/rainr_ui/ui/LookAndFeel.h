#pragma once

namespace rainr::ui
{
/** The Rainr Audio hardware look: machined knobs with a glossy cap, a value ring (accent red with
    a glow for the main control, grey for the rest) and pill toggles with an LED.

    Knob style per slider: slider.getProperties().set (LookAndFeel::bigKnob, true). */
class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    static inline const juce::Identifier bigKnob { "rainrBigKnob" };
    /** Centre-zero knobs (Tone, gains): the ring grows from 12 o'clock. */
    static inline const juce::Identifier bipolarKnob { "rainrBipolarKnob" };

    LookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;

    /** Recessed panel (wells, trays): dark fill, shadow along the top, faint highlight below. */
    static void drawRecess (juce::Graphics&, juce::Rectangle<float> area, float cornerRadius);

    /** Letter-spaced monospaced caps label, as on the logo sheet. */
    static void drawCapsLabel (juce::Graphics&, const juce::String&, juce::Rectangle<float> area,
                               juce::Justification, float height = 11.0f, juce::Colour = colours::label);

private:
    juce::SharedResourcePointer<Typefaces> typefaces; // keeps the fonts loaded while this UI exists
};

/** Rotary knob preset: 270° travel, vertical/horizontal drag, double-click resets to default. */
void configureKnob (juce::Slider&, bool big, bool bipolar = false);
} // namespace rainr::ui
