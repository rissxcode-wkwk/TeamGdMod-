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

# Library FFmpeg dibangun oleh scripts/build-ffmpeg-*.sh. Yang di-link hanya library yang benar-benar ada,
# jadi build tanpa x265/aom/vpx tetap jalan.
function(ffmpeg_collect_libs DIR PREFIX SUFFIX OUTVAR)
    set(_libs "")
    foreach(ffmpeg_lib avformat avcodec avfilter swscale swresample postproc avutil x264 x265 aom vpx openh264)
        foreach(_name ${PREFIX}${ffmpeg_lib}${SUFFIX} lib${ffmpeg_lib}${SUFFIX})
            if (EXISTS ${DIR}/${_name})
                message(STATUS "FFmpeg: link ${_name}")
                list(APPEND _libs ${DIR}/${_name})
                break()
            endif()
        endforeach()
    endforeach()
    set(${OUTVAR} ${_libs} PARENT_SCOPE)
endfunction()

if (WIN32)
    ffmpeg_collect_libs(${CMAKE_SOURCE_DIR}/lib "" ".lib" FFMPEG_FOUND_LIBS)
    if (NOT FFMPEG_FOUND_LIBS)
        message(FATAL_ERROR "Library FFmpeg (.lib) tidak ditemukan di ${CMAKE_SOURCE_DIR}/lib. Jalankan scripts/build-ffmpeg-windows.sh dulu.")
    endif()
    # Library sistem Windows (dipanggil dengan nama, bukan path file)
    set(STATIC_LIBS ${FFMPEG_FOUND_LIBS} ws2_32 secur32 crypt32 bcrypt ole32 user32 mfplat mf mfuuid strmiids)
elseif (ANDROID)
    # ANDROID_ABI belum tentu diset oleh toolchain Geode 6, jadi deteksi dengan beberapa cara.
    set(FFMPEG_ABI "${ANDROID_ABI}")
    if (NOT FFMPEG_ABI)
        set(FFMPEG_ABI "${CMAKE_ANDROID_ARCH_ABI}")
    endif()
    if (NOT FFMPEG_ABI AND CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(FFMPEG_ABI "arm64-v8a")
    endif()
    if (NOT FFMPEG_ABI AND CMAKE_SYSTEM_PROCESSOR MATCHES "armv7|arm")
        set(FFMPEG_ABI "armeabi-v7a")
    endif()
    message(STATUS "FFmpeg: ANDROID_ABI='${ANDROID_ABI}', ABI yang dipakai='${FFMPEG_ABI}'")
    if (FFMPEG_ABI STREQUAL "arm64-v8a" OR FFMPEG_ABI STREQUAL "armeabi-v7a")
        if (FFMPEG_ABI STREQUAL "armeabi-v7a")
            add_definitions(-D__USE_FILE_OFFSET64)
        endif()
        set(FFMPEG_LIB_DIR ${CMAKE_SOURCE_DIR}/lib/${FFMPEG_ABI})
        if (NOT EXISTS ${FFMPEG_LIB_DIR}/libavcodec.a)
            message(FATAL_ERROR "libavcodec.a tidak ditemukan di ${FFMPEG_LIB_DIR}. Jalankan ABI=${FFMPEG_ABI} scripts/build-ffmpeg-android.sh dulu.")
        endif()
        ffmpeg_collect_libs(${FFMPEG_LIB_DIR} "" ".a" FFMPEG_FOUND_LIBS)
        set(STATIC_LIBS -Wl,--start-group ${FFMPEG_FOUND_LIBS} -Wl,--end-group android mediandk log z)
    else()
        message(FATAL_ERROR "ABI Android tidak didukung/terdeteksi: '${FFMPEG_ABI}'")
    endif()
else ()
    # macOS: library universal (arm64 + x86_64) hasil scripts/build-ffmpeg-macos.sh
    ffmpeg_collect_libs(${CMAKE_SOURCE_DIR}/lib "" ".a" FFMPEG_FOUND_LIBS)
    if (NOT FFMPEG_FOUND_LIBS)
        message(FATAL_ERROR "Library FFmpeg (.a) tidak ditemukan di ${CMAKE_SOURCE_DIR}/lib. Jalankan scripts/build-ffmpeg-macos.sh dulu.")
    endif()
    set(STATIC_LIBS
        ${FFMPEG_FOUND_LIBS}
        "-lm -lz -liconv -pthread -framework Security -framework VideoToolbox -framework CoreFoundation -framework CoreMedia -framework CoreVideo -framework CoreServices -framework CoreAudio -framework AVFoundation -framework CoreGraphics -framework AudioToolbox -framework Metal -framework CoreImage -framework AppKit"
    )
endif()
if (NOT STATIC_LIBS)
    message(FATAL_ERROR "Tidak ada library FFmpeg yang akan di-link (STATIC_LIBS kosong). Platform/ABI tidak terdeteksi.")
endif()
target_link_libraries(${PROJECT_NAME} ${STATIC_LIBS})

if (ANDROID)
    # Simbol FFmpeg yang didefinisikan di dalam .so di-bind secara lokal (non-preemptible),
    # sehingga relokasi adrp/add dari asm NEON FFmpeg (tx_float_neon.S) tidak ditolak lld.
    # Dipasang lewat dua jalur supaya pasti masuk ke perintah link.
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,-Bsymbolic)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-Bsymbolic")
    message(STATUS "FFmpeg: -Wl,-Bsymbolic aktif untuk Android")
endif()

if (PROJECT_IS_TOP_LEVEL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE FFMPEG_API_EXPORTING)
endif()
