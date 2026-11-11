#pragma once

#include <string>
#include <asp/fs/Path.hpp>

namespace ffmpeg::utils {

void customLogCallback(void* ptr, int level, const char* fmt, char* vargs);

std::string getErrorString(int errorCode);

// FFmpeg butuh string C yang diakhiri null; PathView tidak menjamin itu, jadi disalin ke std::string.
inline std::string pathString(asp::fs::PathView path) {
    return std::string(path.view());
}

}