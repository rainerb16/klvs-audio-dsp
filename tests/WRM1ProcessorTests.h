#pragma once

#include <juce_core/juce_core.h>

/** Processor-level tests (WRM1ProcessorTests.cpp); reports through the given check function. */
int runProcessorTests (void (*check) (bool, const juce::String&));
