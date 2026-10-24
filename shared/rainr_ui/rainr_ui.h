/*
BEGIN_JUCE_MODULE_DECLARATION

 ID:                 rainr_ui
 vendor:             Rainr Audio
 version:            0.1.0
 name:               Rainr Audio shared UI
 description:        Brand colours, fonts, knobs, meters and displays shared by Rainr Audio plugins.
 license:            Proprietary
 minimumCppStandard: 20
 dependencies:       juce_gui_basics, juce_audio_processors

END_JUCE_MODULE_DECLARATION
*/

#pragma once
#define RAINR_UI_H_INCLUDED

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

// Targets using this module must also link the RainrUiFonts binary data target (done by
// rainr_add_plugin() in cmake/RainrPlugin.cmake).

#include "ui/Palette.h"
#include "ui/Fonts.h"
#include "ui/LookAndFeel.h"
#include "ui/LevelMeter.h"
#include "ui/SegmentedControl.h"
#include "ui/TransferScope.h"
