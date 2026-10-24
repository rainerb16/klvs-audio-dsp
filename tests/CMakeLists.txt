juce_add_console_app(WRM1Tests PRODUCT_NAME "WRM1Tests" VERSION 0.0.0)

file(STRINGS ${CMAKE_SOURCE_DIR}/plugins/WRM1/VERSION WRM1_VERSION LIMIT_COUNT 1)
set(WRM1_SRC ${CMAKE_SOURCE_DIR}/plugins/WRM1/Source)

# The engine tests need only Engine.cpp; the processor tests (parameters, saved state, bypass)
# compile the plugin's processor and editor sources directly, as the real plugin would.
target_sources(WRM1Tests PRIVATE
    WRM1EngineTests.cpp
    WRM1ProcessorTests.cpp
    ${WRM1_SRC}/Engine.cpp
    ${WRM1_SRC}/PluginProcessor.cpp
    ${WRM1_SRC}/PluginEditor.cpp
    ${WRM1_SRC}/Parameters.cpp)

target_include_directories(WRM1Tests PRIVATE ${WRM1_SRC})

target_compile_definitions(WRM1Tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JucePlugin_Name="WRM1"
    RAINR_PLUGIN_VERSION="${WRM1_VERSION}"
    RAINR_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden")

target_link_libraries(WRM1Tests PRIVATE
    rainr_dsp
    rainr_ui
    RainrUiFonts
    WRM1Assets
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

add_test(NAME WRM1Engine COMMAND WRM1Tests)
