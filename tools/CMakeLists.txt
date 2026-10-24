# WRM1Snapshot: renders the WRM1 window to a PNG without a DAW, for checking the UI.
#   WRM1Snapshot out.png [param=value ...]     e.g. warmth=0.7 character=0
juce_add_console_app(WRM1Snapshot PRODUCT_NAME "WRM1Snapshot" VERSION 0.0.0)

file(STRINGS ${CMAKE_SOURCE_DIR}/plugins/WRM1/VERSION WRM1_VERSION LIMIT_COUNT 1)
set(WRM1_SRC ${CMAKE_SOURCE_DIR}/plugins/WRM1/Source)

target_sources(WRM1Snapshot PRIVATE
    WRM1Snapshot.cpp
    ${WRM1_SRC}/PluginProcessor.cpp
    ${WRM1_SRC}/PluginEditor.cpp
    ${WRM1_SRC}/Parameters.cpp
    ${WRM1_SRC}/Engine.cpp)

target_include_directories(WRM1Snapshot PRIVATE ${WRM1_SRC})

target_compile_definitions(WRM1Snapshot PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=1
    JucePlugin_Name="WRM1"
    RAINR_PLUGIN_VERSION="${WRM1_VERSION}")

target_link_libraries(WRM1Snapshot PRIVATE
    rainr_dsp
    rainr_ui
    RainrUiFonts
    WRM1Assets
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
