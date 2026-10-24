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

// Filled in Phase 2 step 3 (DSP port of docs/WRM1_DSP.md):
// saturation curves, RBJ biquads, DC blocker, parameter smoothing, auto gain table lookup,
// oversampling wrapper. Nothing in this module may allocate on the audio thread.
namespace rainr::dsp
{
}
