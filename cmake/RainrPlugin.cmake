# rainr_add_plugin(): the one place where company-wide plugin settings live.
#
#   rainr_add_plugin(<target>
#       PRODUCT_NAME <name shown in the DAW>
#       PLUGIN_CODE  <4 chars, exactly one upper-case letter>   # PERMANENT, see below
#       BUNDLE_ID    <com.rainraudio.xxx>                        # PERMANENT
#       VERSION      <x.y.z>
#       VST3_CATEGORIES <...>
#       SOURCES      <files...>)

# ----------------------------------------------------------------------------------------------
#  PERMANENT IDENTIFIERS. Hosts identify a plugin by manufacturer code + plugin code (AU) and by
#  an ID derived from them (VST3). Changing either makes every saved project lose the plugin.
#  The registry of codes lives in docs/CONVENTIONS.md.
# ----------------------------------------------------------------------------------------------
set(RAINR_COMPANY_NAME "Rainr Audio")
set(RAINR_MANUFACTURER_CODE "Rnra")

function(rainr_add_plugin target)
    cmake_parse_arguments(ARG "" "PRODUCT_NAME;PLUGIN_CODE;BUNDLE_ID;VERSION" "VST3_CATEGORIES;SOURCES" ${ARGN})

    juce_add_plugin(${target}
        VERSION "${ARG_VERSION}"
        COMPANY_NAME "${RAINR_COMPANY_NAME}"
        BUNDLE_ID "${ARG_BUNDLE_ID}"
        PLUGIN_MANUFACTURER_CODE ${RAINR_MANUFACTURER_CODE}
        PLUGIN_CODE ${ARG_PLUGIN_CODE}
        PRODUCT_NAME "${ARG_PRODUCT_NAME}"
        FORMATS AU VST3
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE   # never touch the system plugin folders from a build
        VST3_CATEGORIES ${ARG_VST3_CATEGORIES}
        AU_MAIN_TYPE kAudioUnitType_Effect)

    target_sources(${target} PRIVATE ${ARG_SOURCES})

    target_compile_definitions(${target}
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            RAINR_PLUGIN_VERSION="${ARG_VERSION}")

    target_link_libraries(${target}
        PRIVATE
            rainr_dsp
            rainr_ui
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endfunction()
