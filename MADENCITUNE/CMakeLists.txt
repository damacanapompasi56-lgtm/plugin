cmake_minimum_required(VERSION 3.22)
project(MADENCITUNE VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 7.0.12
  GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(MadenciTune
  COMPANY_NAME "Madenci Audio"
  PLUGIN_MANUFACTURER_CODE Mdnc
  PLUGIN_CODE Mtn1
  FORMATS VST3 Standalone
  PRODUCT_NAME "MADENCITUNE"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES Fx "Pitch Shift")

juce_add_binary_data(MadenciAssets SOURCES Assets/photo1.png Assets/photo2.png)

target_sources(MadenciTune PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp)

target_compile_definitions(MadenciTune PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0)

juce_generate_juce_header(MadenciTune)

target_link_libraries(MadenciTune
  PRIVATE MadenciAssets juce::juce_audio_utils
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)

if(MSVC)
  target_compile_options(MadenciTune PRIVATE /utf-8)
endif()
