#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace wrm1::params
{
// =============================================================================================
//  PARAMETER IDs ARE PERMANENT.
//
//  Saved Ableton sets (and every other host) store parameter values by these ID strings.
//  Renaming or removing an ID silently resets that control in every project that uses WRM1.
//
//  - Never rename an ID, never change its range or default meaning.
//  - Choice parameters are stored as an index: never reorder or remove choices, only append.
//  - New parameters get a new ID and the version hint of the release that adds them.
//  - The version hint of an existing parameter never changes.
//
//  Spec: docs/WRM1_DSP.md section 1. Conventions: docs/CONVENTIONS.md.
// =============================================================================================
inline constexpr const char* input        = "input";
inline constexpr const char* warmth       = "warmth";
inline constexpr const char* tone         = "tone";
inline constexpr const char* body         = "body";
inline constexpr const char* character    = "character";
inline constexpr const char* lowProtect   = "lowProtect";
inline constexpr const char* mix          = "mix";
inline constexpr const char* autoGain     = "autoGain";
inline constexpr const char* output       = "output";
inline constexpr const char* oversampling = "oversampling";

// Version hint for every parameter that shipped in v0.1.0.
inline constexpr int firstVersionHint = 1;

// Choice order is permanent (stored as an index).
inline const juce::StringArray characterChoices    { "Tape", "Tube", "Console" };
inline const juce::StringArray oversamplingChoices { "Off", "2x", "4x" };

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
} // namespace wrm1::params
