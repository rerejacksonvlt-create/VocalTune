
cmake_minimum_required(VERSION 3.22)

project(VocalTune VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# JUCE framework
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.10
)

FetchContent_MakeAvailable(JUCE)

# Signalsmith Linear dependency
FetchContent_Declare(
    signalsmith_linear
    GIT_REPOSITORY https://github.com/Signalsmith-Audio/linear.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(signalsmith_linear)

# Signalsmith Stretch pitch-shifting engine
FetchContent_Declare(
    signalsmith_stretch
    GIT_REPOSITORY https://github.com/Signalsmith-Audio/signalsmith-stretch.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    GIT_SUBMODULES ""
)

FetchContent_GetProperties(signalsmith_stretch)

if(NOT signalsmith_stretch_POPULATED)
    FetchContent_Populate(signalsmith_stretch)
endif()

juce_add_plugin(VocalTune
    COMPANY_NAME "VocalTune"
    BUNDLE_ID "com.vocaltune.plugin"
    PLUGIN_MANUFACTURER_CODE Vtne
    PLUGIN_CODE Vt01
    FORMATS VST3
    PRODUCT_NAME "VocalTune"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

juce_generate_juce_header(VocalTune)

target_sources(VocalTune
    PRIVATE
        PluginProcessor.cpp
        PluginProcessor.h
        PluginEditor.cpp
        PluginEditor.h
)

target_include_directories(VocalTune
    PRIVATE
        "${signalsmith_stretch_SOURCE_DIR}"
)

target_compile_definitions(VocalTune
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(VocalTune
    PRIVATE
        juce::juce_audio_utils
        signalsmith-linear
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
