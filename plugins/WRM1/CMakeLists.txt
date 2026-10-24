file(STRINGS VERSION WRM1_VERSION LIMIT_COUNT 1)

rainr_add_plugin(WRM1
    PRODUCT_NAME "WRM1"
    PLUGIN_CODE Wrm1                 # PERMANENT (docs/CONVENTIONS.md)
    BUNDLE_ID com.rainraudio.wrm1    # PERMANENT
    VERSION ${WRM1_VERSION}
    VST3_CATEGORIES Fx Distortion
    SOURCES
        Source/PluginProcessor.cpp
        Source/Parameters.cpp
        Source/Engine.cpp
        Source/PluginEditor.cpp
    ASSETS
        Assets/lockup.png)
