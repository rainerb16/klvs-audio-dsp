/*
BEGIN_JUCE_MODULE_DECLARATION

 ID:                 rainr_dsp
 vendor:             Rainr Audio
 version:            0.1.0
 name:               Rainr Audio shared DSP
 description:        Saturation curves, filters, smoothing, auto gain and oversampling shared by Rainr Audio plugins.
 license:            Proprietary
 minimumCppStandard: 20
 dependencies:       juce_dsp

END_JUCE_MODULE_DECLARATION
*/

#pragma once
#define RAINR_DSP_H_INCLUDED

#include <juce_dsp/juce_dsp.h>

// Building blocks shared by Rainr Audio plugins. Nothing here allocates; everything is safe to
// call on the audio thread once constructed.
#include "dsp/Biquad.h"
#include "dsp/Saturation.h"
