cmake_minimum_required(VERSION 3.21)
add_library(third_party INTERFACE)

# CMake 4.x menghapus kompatibilitas < 3.5; beberapa dependensi lama (discord-rpc, dll.) masih memakainya
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

# Include libraries
# ImGui: fork EclipseMenu/imgui sudah tidak tersedia, memakai ImGui resmi (sama dengan versi yang
# dipakai gd-imgui-cocos terbaru yang mendukung Geode v6 / GD 2.2091).
set(IMGUI_VERSION "v1.92.4")
CPMAddPackage("gh:ocornut/imgui#${IMGUI_VERSION}")
add_library(imgui INTERFACE)
target_include_directories(imgui INTERFACE ${imgui_SOURCE_DIR})
target_sources(imgui INTERFACE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)
set(HAS_IMGUI ON)

# Discord RPC: fork EclipseMenu/discord-rpc sudah tidak tersedia, memakai upstream resmi
if (NOT ANDROID)
    set(RAPIDJSONTEST OFF)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    CPMAddPackage(
        NAME discord-rpc
        GITHUB_REPOSITORY discord/discord-rpc
        GIT_TAG 963aa9f
        DOWNLOAD_ONLY YES
    )
    # .clang-format bawaan discord-rpc rusak (key ganda) dan membuat build gagal di Windows,
    # jadi diganti dengan versi minimal yang valid sebelum di-add.
    file(WRITE "${discord-rpc_SOURCE_DIR}/.clang-format" "BasedOnStyle: LLVM\n")
    add_subdirectory("${discord-rpc_SOURCE_DIR}" "${discord-rpc_BINARY_DIR}")
endif()

# imgui-cocos terbaru (mendukung Geode v6). Dinonaktifkan hook awal karena ini mod menu.
set(IMGUI_HOOK_EARLY OFF CACHE BOOL "" FORCE)
CPMAddPackage("gh:matcool/gd-imgui-cocos#934958c")
CPMAddPackage("gh:maxnut/GDReplayFormat#4950cc2")
# RIFT: memakai implementasi lokal di third_party/rift (header-only), tidak perlu diunduh
CPMAddPackage("gh:SpaghettDev/subprocess#e12740b")
CPMAddPackage(
    NAME nlohmann_json
    GITHUB_REPOSITORY nlohmann/json
    VERSION 3.11.3
)

# Fix debug build
if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND WIN32)
    target_compile_definitions(discord-rpc PRIVATE _ITERATOR_DEBUG_LEVEL=0)
endif()

# Link them to the project
target_link_libraries(third_party INTERFACE
    libGDR
    subprocess
    imgui
    imgui-cocos
    nlohmann_json::nlohmann_json
)

if (NOT ANDROID)
    target_link_libraries(third_party INTERFACE discord-rpc)
endif()