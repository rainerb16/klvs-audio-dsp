juce_add_console_app(WRM1Tests PRODUCT_NAME "WRM1Tests" VERSION 0.0.0)

target_sources(WRM1Tests PRIVATE
    WRM1EngineTests.cpp
    ${CMAKE_SOURCE_DIR}/plugins/WRM1/Source/Engine.cpp)

target_include_directories(WRM1Tests PRIVATE ${CMAKE_SOURCE_DIR}/plugins/WRM1/Source)

target_compile_definitions(WRM1Tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    RAINR_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden")

target_link_libraries(WRM1Tests PRIVATE
    rainr_dsp
    juce::juce_audio_formats
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

add_test(NAME WRM1Engine COMMAND WRM1Tests)
