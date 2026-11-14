#pragma once

// Deteksi iOS yang tidak bergantung pada macro Geode (GEODE_IS_IOS bisa tidak
// terdefinisi di fork/versi SDK tertentu).
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(GEODE_IS_IOS) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
#define JB_IS_IOS 1
#endif

#if !defined(JB_IS_IOS) && defined(__APPLE__)
#define JB_IS_MACOS 1
#endif
