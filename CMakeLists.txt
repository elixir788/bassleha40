cmake_minimum_required(VERSION 3.22)

# macOS: универсальная сборка (Intel + Apple Silicon)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

project(BassOtLehi40 VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

if(DEFINED ENV{CI})
    set(COPY_AFTER FALSE)
else()
    set(COPY_AFTER TRUE)
endif()

juce_add_plugin(BassOtLehi40
    COMPANY_NAME "Lehi40"
    PLUGIN_MANUFACTURER_CODE Leh4
    PLUGIN_CODE Bs40
    FORMATS VST3 Standalone
    PRODUCT_NAME "Obrabotka Bassa ot Lehi40"
    VST3_CATEGORIES Fx EQ
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD ${COPY_AFTER})

target_sources(BassOtLehi40 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(BassOtLehi40 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

# Нужно, чтобы русский текст в исходниках читался правильно
target_compile_options(BassOtLehi40 PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(BassOtLehi40
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
