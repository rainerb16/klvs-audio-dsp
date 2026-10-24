/*
BEGIN_JUCE_MODULE_DECLARATION

 ID:                 rainr_ui
 vendor:             Rainr Audio
 version:            0.1.0
 name:               Rainr Audio shared UI
 description:        Brand colours, fonts, knobs and meters shared by Rainr Audio plugins.
 license:            Proprietary
 minimumCppStandard: 20
 dependencies:       juce_gui_basics

END_JUCE_MODULE_DECLARATION
*/

#pragma once
#define RAINR_UI_H_INCLUDED

#include <juce_gui_basics/juce_gui_basics.h>

namespace rainr::ui
{
// Brand palette, taken from the Rainr Audio logo sheet.
namespace colours
{
    inline const juce::Colour background { 0xff0a0a0b };
    inline const juce::Colour panel      { 0xff121214 };
    inline const juce::Colour border     { 0xff2a2a2e };
    inline const juce::Colour text       { 0xffecebe9 };
    inline const juce::Colour grey       { 0xffc2c2c6 };
    inline const juce::Colour muted      { 0xff8e8e93 };
    inline const juce::Colour red        { 0xffe30613 };
} // namespace colours

// Knobs, meters, fonts and the scope arrive in Phase 2 step 4; for now this only applies the
// palette to JUCE's stock widgets.
class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
};
} // namespace rainr::ui
