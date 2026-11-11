cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

project(ffmpeg-api VERSION 1.0.0)
message(STATUS "ffmpeg-api CMake rev2 (Geode 6): deteksi ABI + cek library FFmpeg aktif")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
)

setup_geode_mod(${PROJECT_NAME})

if (WIN32)
    set(STATIC_LIBS 
        ${CMAKE_SOURCE_DIR}/lib/avcodec.lib
        ${CMAKE_SOURCE_DIR}/lib/avformat.lib
        ${CMAKE_SOURCE_DIR}/lib/avutil.lib
        ${CMAKE_SOURCE_DIR}/lib/avfilter.lib
        ${CMAKE_SOURCE_DIR}/lib/swresample.lib
        ${CMAKE_SOURCE_DIR}/lib/swscale.lib
        ${CMAKE_SOURCE_DIR}/lib/libx264.lib
        ${CMAKE_SOURCE_DIR}/lib/x265-static.lib
        ${CMAKE_SOURCE_DIR}/lib/aom.lib
        ${CMAKE_SOURCE_DIR}/lib/vpx.lib
        ${CMAKE_SOURCE_DIR}/lib/openh264.lib
        ${CMAKE_SOURCE_DIR}/lib/ws2_32.lib
        ${CMAKE_SOURCE_DIR}/lib/mfplat.lib
        ${CMAKE_SOURCE_DIR}/lib/mf.lib
        ${CMAKE_SOURCE_DIR}/lib/secur32.lib
        ${CMAKE_SOURCE_DIR}/lib/crypt32.lib
        ${CMAKE_SOURCE_DIR}/lib/mfuuid.lib
        ${CMAKE_SOURCE_DIR}/lib/bcrypt.lib
        ${CMAKE_SOURCE_DIR}/lib/strmiids.lib
    ) # windows
elseif (ANDROID)
    # ANDROID_ABI belum tentu diset oleh toolchain Geode 6, jadi deteksi dengan beberapa cara.
    set(FFMPEG_ABI "${ANDROID_ABI}")
    if (NOT FFMPEG_ABI)
        set(FFMPEG_ABI "${CMAKE_ANDROID_ARCH_ABI}")
    endif()
    if (NOT FFMPEG_ABI AND CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(FFMPEG_ABI "arm64-v8a")
    endif()
    message(STATUS "FFmpeg: ANDROID_ABI='${ANDROID_ABI}', ABI yang dipakai='${FFMPEG_ABI}'")
    if (FFMPEG_ABI STREQUAL "arm64-v8a")
        # Library FFmpeg dibangun oleh scripts/build-ffmpeg-android.sh ke lib/arm64-v8a.
        # Yang dipakai hanya library yang benar-benar ada, jadi build tanpa x265/aom/vpx tetap jalan.
        set(FFMPEG_LIB_DIR ${CMAKE_SOURCE_DIR}/lib/arm64-v8a)
        if (NOT EXISTS ${FFMPEG_LIB_DIR}/libavcodec.a)
            message(FATAL_ERROR "libavcodec.a tidak ditemukan di ${FFMPEG_LIB_DIR}. Jalankan scripts/build-ffmpeg-android.sh dulu.")
        endif()
        set(STATIC_LIBS -Wl,--start-group)
        foreach(ffmpeg_lib avformat avcodec avfilter swscale swresample postproc avutil x264 x265 aom vpx)
            if (EXISTS ${FFMPEG_LIB_DIR}/lib${ffmpeg_lib}.a)
                message(STATUS "FFmpeg: link lib${ffmpeg_lib}.a")
                list(APPEND STATIC_LIBS ${FFMPEG_LIB_DIR}/lib${ffmpeg_lib}.a)
            endif()
        endforeach()
        list(APPEND STATIC_LIBS -Wl,--end-group android mediandk log z)
        # Pengaman tambahan: simbol yang didefinisikan di dalam .so di-bind secara lokal,
        # sehingga relokasi adrp/add dari asm NEON FFmpeg tidak ditolak linker.
        target_link_options(${PROJECT_NAME} PRIVATE -Wl,-Bsymbolic)
    elseif(FFMPEG_ABI STREQUAL "armeabi-v7a")
        add_definitions(-D__USE_FILE_OFFSET64)
        add_definitions(-DANDROID_PLATFORM=24)
        set(STATIC_LIBS
            ${CMAKE_SOURCE_DIR}/lib/armeabi-v7a/libavcodec.a
            ${CMAKE_SOURCE_DIR}/lib/armeabi-v7a/libavformat.a
            ${CMAKE_SOURCE_DIR}/lib/armeabi-v7a/libavutil.a
            ${CMAKE_SOURCE_DIR}/lib/armeabi-v7a/libavfilter.a
            ${CMAKE_SOURCE_DIR}/lib/armeabi-v7a/libswresample.a
            ${CMAKE_SOURCE_DIR}/lib/armeabi-v7a/libswscale.a
            ${CMAKE_SOURCE_DIR}/lib/armeabi-v7a/libpostproc.a
            ${CMAKE_SOURCE_DIR}/lib/armeabi-v7a/libaom.a
            ${CMAKE_SOURCE_DIR}/lib/armeabi-v7a/libvpx.a
            ${CMAKE_SOURCE_DIR}/lib/armeabi-v7a/libx264.a
            ${CMAKE_SOURCE_DIR}/lib/armeabi-v7a/libx265.a
            android
            mediandk
            log
        ) # android
    else()
        message(FATAL_ERROR "ABI Android tidak didukung/terdeteksi: '${FFMPEG_ABI}'")
    endif()
else ()
    find_package(BZIP2 REQUIRED)
    set(STATIC_LIBS 
        "-framework Security"
        ${CMAKE_SOURCE_DIR}/lib/libx264.a
        ${CMAKE_SOURCE_DIR}/lib/libavcodec.a
        ${CMAKE_SOURCE_DIR}/lib/libavformat.a
        ${CMAKE_SOURCE_DIR}/lib/libavutil.a
        ${CMAKE_SOURCE_DIR}/lib/libavfilter.a
        ${CMAKE_SOURCE_DIR}/lib/libpostproc.a
        ${CMAKE_SOURCE_DIR}/lib/libswresample.a
        ${CMAKE_SOURCE_DIR}/lib/libswscale.a
        ${CMAKE_SOURCE_DIR}/lib/liblzma.a   
        ${BZIP2_LIBRARY}
        "-lm -lbz2 -lz -liconv -pthread -framework Security -framework VideoToolbox -framework CoreFoundation -framework CoreMedia -framework CoreVideo -framework CoreServices -framework CoreAudio -framework AVFoundation -framework CoreGraphics -framework AudioToolbox -framework Metal -framework CoreImage -framework AppKit"
    ) # macos
endif()
if (NOT STATIC_LIBS)
    message(FATAL_ERROR "Tidak ada library FFmpeg yang akan di-link (STATIC_LIBS kosong). Platform/ABI tidak terdeteksi.")
endif()
target_link_libraries(${PROJECT_NAME} ${STATIC_LIBS})

if (PROJECT_IS_TOP_LEVEL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE FFMPEG_API_EXPORTING)
endif()
