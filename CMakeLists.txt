cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(IOS TRUE)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0")
    add_definitions(-DGLES_SILENCE_DEPRECATION)
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(EclipseMenu VERSION 1.0.0 LANGUAGES CXX)

# Sources
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Configure for GitHub Actions builds
if (DEFINED ENV{GITHUB_ACTIONS})
  # Enable LTO for smaller binaries
  set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE GITHUB_ACTIONS)
  message(STATUS "GitHub Actions build detected")
  set(ECLIPSE_DISABLE_INCREMENTAL_LINK ON)
  set(ECLIPSE_ENABLE_UNITY_BUILD ON)
endif()

if (ECLIPSE_ENABLE_UNITY_BUILD)
  message(STATUS "Enabling unity build")
  set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    UNITY_BUILD ON
    UNITY_BUILD_UNIQUE_ID "GEODE_UNITY_NS_ID"
  )
endif()

# Make sure we can include headers from src
target_include_directories(${PROJECT_NAME} PRIVATE src include)

# --- iOS: bindings 2.2091 belum punya alamat iOS untuk sebagian fungsi/ctor/dtor ---
# Header modify bawaan gagal ("does not have an available address" / "expected expression").
# Untuk iOS dibuat shim `Geode/modify/<Kelas>.hpp` (di folder build) yang mematikan pengecekan itu lalu
# memanggil header asli lewat #include_next. Hook untuk fungsi tanpa alamat dilewati di iOS.
if (IOS OR "${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR "${GEODE_TARGET_PLATFORM}" STREQUAL "iOS")
  set(ECLIPSE_IOS_SHIM_DIR "${CMAKE_CURRENT_BINARY_DIR}/ios-shims")
  set(ECLIPSE_IOS_SHIM_CLASSES
    CCCircleWave
    CCDelayTime
    CCDirector
    CCDrawNode
    CCEGLView
    CCEGLViewProtocol
    CCKeyboardDispatcher
    CCLayerColor
    CCLightFlash
    CCMotionStreak
    CCParticleSystem
    CCParticleSystemQuad
    CCScene
    CCScheduler
    CCTextInputNode
    CCTouchDispatcher
    CCTransitionFade
    CheckpointObject
    CommentCell
    CreatorLayer
    CustomListView
    EditLevelLayer
    EditorPauseLayer
    EditorUI
    EffectGameObject
    EndLevelLayer
    EnhancedGameObject
    FMODAudioEngine
    GJBaseGameLayer
    GJScaleControl
    GameLevelManager
    GameManager
    GameObject
    GameStatsManager
    GameToolbox
    HardStreak
    LevelCell
    LevelEditorLayer
    LevelInfoLayer
    LevelPage
    LevelSettingsObject
    LevelTools
    MenuGameLayer
    MenuLayer
    MoreSearchLayer
    OBB2D
    OptionsLayer
    PauseLayer
    PlayLayer
    PlayerObject
    RetryLevelLayer
    RewardUnlockLayer
    SecretLayer2
    ShaderLayer
    SliderTouchLogic
    SongSelectNode
    UILayer
  )
  set(ECLIPSE_IOS_SHIM_TEMPLATE [=[
#pragma once
#include <Geode/platform/platform.hpp>
#if defined(GEODE_IS_IOS)
    #include <Geode/modify/Modify.hpp>
    #pragma push_macro("GEODE_APPLY_MODIFY_FOR_FUNCTION_ERROR_DEFINED")
    #pragma push_macro("GEODE_APPLY_MODIFY_FOR_CONSTRUCTOR")
    #pragma push_macro("GEODE_APPLY_MODIFY_FOR_DESTRUCTOR")
    #undef GEODE_APPLY_MODIFY_FOR_FUNCTION_ERROR_DEFINED
    #undef GEODE_APPLY_MODIFY_FOR_CONSTRUCTOR
    #undef GEODE_APPLY_MODIFY_FOR_DESTRUCTOR
    #define GEODE_APPLY_MODIFY_FOR_FUNCTION_ERROR_DEFINED(...)
    #define GEODE_APPLY_MODIFY_FOR_CONSTRUCTOR(...)
    #define GEODE_APPLY_MODIFY_FOR_DESTRUCTOR(...)
#endif
#include_next <Geode/modify/@CLASS@.hpp>
#if defined(GEODE_IS_IOS)
    #pragma pop_macro("GEODE_APPLY_MODIFY_FOR_FUNCTION_ERROR_DEFINED")
    #pragma pop_macro("GEODE_APPLY_MODIFY_FOR_CONSTRUCTOR")
    #pragma pop_macro("GEODE_APPLY_MODIFY_FOR_DESTRUCTOR")
#endif
]=])
  foreach(ECLIPSE_CLASS IN LISTS ECLIPSE_IOS_SHIM_CLASSES)
    string(REPLACE "@CLASS@" "${ECLIPSE_CLASS}" ECLIPSE_SHIM_CONTENT "${ECLIPSE_IOS_SHIM_TEMPLATE}")
    file(WRITE "${ECLIPSE_IOS_SHIM_DIR}/Geode/modify/${ECLIPSE_CLASS}.hpp" "${ECLIPSE_SHIM_CONTENT}")
  endforeach()
  target_include_directories(${PROJECT_NAME} BEFORE PRIVATE "${ECLIPSE_IOS_SHIM_DIR}")
  message(STATUS "iOS: shim header modify dibuat di ${ECLIPSE_IOS_SHIM_DIR}")
endif()

# Get the latest abbreviated commit hash of the working branch
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
target_compile_definitions(${PROJECT_NAME} PRIVATE GIT_HASH="${GIT_HASH}")

# Windows specific settings
if (WIN32)
  # Disable annoying warnings
  target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

  # Export all symbols for better debugging
  # UPD: this bloats the binary size, so it's disabled for now
  # (also it doesn't work for every crashlog anyways)
  # set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

  # Disable incremental linking if requested
  if (DEFINED ECLIPSE_DISABLE_INCREMENTAL_LINK)
    message(STATUS "Disabling incremental linking")
    if (MSVC)
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /INCREMENTAL:NO")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /INCREMENTAL:NO")
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-incremental:no")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-incremental:no")
    endif()
  endif()
endif()

# Debug macro
if (NOT DEFINED ENV{GITHUB_ACTIONS})
  target_compile_definitions(
    ${PROJECT_NAME} PRIVATE
      $<$<CONFIG:Debug>:ECLIPSE_DEBUG_BUILD>
      $<$<CONFIG:RelWithDebInfo>:ECLIPSE_DEBUG_BUILD>
  )
endif()

# Add Geode
if (NOT DEFINED ENV{GEODE_SDK})
  message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
  message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Libraries
include(${CMAKE_SOURCE_DIR}/cmake/IncludeLibs.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Translations.cmake)
target_link_libraries(${PROJECT_NAME} third_party)

# Setup Geode
setup_geode_mod(${PROJECT_NAME})
