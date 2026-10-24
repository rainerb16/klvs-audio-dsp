#pragma once

namespace rainr::ui
{
/** The loaded brand typefaces, shared by everything that draws text. Held by the LookAndFeel so
    they load once per UI and are released with it (no static outliving the plugin). */
struct Typefaces;

/** Brand fonts (IBM Plex, SIL Open Font License), embedded so they look the same everywhere.
    Call from the message thread. */
struct Fonts
{
    static juce::Font sans (float height);
    /** Monospaced, for values and letter-spaced caps labels. */
    static juce::Font mono (float height, bool medium = false, float tracking = 0.0f);
};
} // namespace rainr::ui
