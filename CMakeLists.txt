cmake_minimum_required(VERSION 3.22)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Oldest target: macOS Big Sur")
endif()
project(SyllableLeveler VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(SYLLABLE_BUILD_PLUGIN "Build AU/ARA plugin (macOS required)" ${APPLE})
include(CTest)
if(BUILD_TESTING)
    add_executable(leveling_tests Tests/LevelingTests.cpp)
    target_include_directories(leveling_tests PRIVATE Source)
    add_test(NAME leveling_engine COMMAND leveling_tests)
endif()
if(NOT SYLLABLE_BUILD_PLUGIN)
    return()
endif()
if(NOT APPLE)
    message(FATAL_ERROR "AU requires macOS. Use -DSYLLABLE_BUILD_PLUGIN=OFF for engine tests.")
endif()
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6 GIT_SHALLOW TRUE)
FetchContent_Declare(ARA
    GIT_REPOSITORY https://github.com/Celemony/ARA_SDK.git
    GIT_TAG 813995632d7bdecfe8c182e579e155c1214e6692
    GIT_SUBMODULES ARA_API ARA_Library
    GIT_SUBMODULES_RECURSE TRUE)
FetchContent_MakeAvailable(JUCE ARA)
juce_set_ara_sdk_path("${ara_SOURCE_DIR}")
juce_add_plugin(SyllableLeveler
    COMPANY_NAME "Zivko Audio"
    BUNDLE_ID "com.zivkoaudio.syllableleveler"
    PLUGIN_MANUFACTURER_CODE Zivk
    PLUGIN_CODE Sylv
    PRODUCT_NAME "Syllable Leveler"
    FORMATS AU
    AU_MAIN_TYPE kAudioUnitType_Effect
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE IS_ARA_EFFECT TRUE
    ARA_FACTORY_ID "\"com.zivkoaudio.syllableleveler.factory.1\""
    ARA_DOCUMENT_ARCHIVE_ID "\"com.zivkoaudio.syllableleveler.archive.1\""
    ARA_TRANSFORMATION_FLAGS kARAPlaybackTransformationNoChanges
    COPY_PLUGIN_AFTER_BUILD FALSE)
juce_generate_juce_header(SyllableLeveler)
target_sources(SyllableLeveler PRIVATE Source/Plugin.cpp Source/ARAPlugin.h
    Source/AudioModification.h Source/LevelingEngine.h)
target_compile_definitions(SyllableLeveler PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(SyllableLeveler PRIVATE
    juce::juce_audio_utils juce::juce_audio_processors
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
