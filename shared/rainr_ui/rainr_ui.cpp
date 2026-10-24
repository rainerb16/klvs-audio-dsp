#include "rainr_ui.h"

namespace rainr::ui
{
LookAndFeel::LookAndFeel()
{
    using namespace colours;
    setColourScheme ({ background, panel, background, border, text, red, text, red, text });
    setColour (juce::ResizableWindow::backgroundColourId, background);
    setColour (juce::Slider::thumbColourId, red);
    setColour (juce::Slider::trackColourId, grey);
    setColour (juce::Slider::backgroundColourId, border);
}
} // namespace rainr::ui
