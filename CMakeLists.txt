cmake_minimum_required(VERSION 3.22)

project(CrystalVST VERSION 0.1.0)

# Set the standard for C++ (JUCE 8 requires C++17 or later)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE standard library inclusion via FetchContent
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.7
)

FetchContent_MakeAvailable(JUCE)

# Create the plugin target
juce_add_plugin(CrystalVST
    # COMPANY_NAME "MyCompany"
    # PLUGIN_CODE "CrV1"
    # PLUGIN_MANUFACTURER_CODE "MyCo"
    VERSION 0.1.0
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_NAME "CrystalVST"
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "CrystalVST"
)

# Source files
target_sources(CrystalVST PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

# Link modules
target_link_libraries(CrystalVST PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce_recommended_config_flags
    juce_recommended_lto_flags
    juce_recommended_warning_flags
)

# Binary data if needed
# juce_add_binary_data(CrystalVST_Data SOURCES ...)

# Setup output directories (optional but helpful)
set_target_properties(CrystalVST PROPERTIES
    JUCER_VERSION "8.0.0"
)
