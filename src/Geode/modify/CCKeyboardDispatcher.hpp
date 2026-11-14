#pragma once
// Shim khusus iOS: bindings GD 2.2091 tidak punya alamat iOS untuk constructor/destructor
// CCKeyboardDispatcher, sehingga header modify bawaan gagal ("expected expression").
// File ini dicari lebih dulu (lewat -I src) dan menonaktifkan hook ctor/dtor di iOS saja.
#include <Geode/platform/platform.hpp>
#include <Geode/modify/Modify.hpp>

#if defined(GEODE_IS_IOS)
    #pragma push_macro("GEODE_APPLY_MODIFY_FOR_CONSTRUCTOR")
    #pragma push_macro("GEODE_APPLY_MODIFY_FOR_DESTRUCTOR")
    #undef GEODE_APPLY_MODIFY_FOR_CONSTRUCTOR
    #undef GEODE_APPLY_MODIFY_FOR_DESTRUCTOR
    #define GEODE_APPLY_MODIFY_FOR_CONSTRUCTOR(...)
    #define GEODE_APPLY_MODIFY_FOR_DESTRUCTOR(...)
#endif

#include_next <Geode/modify/CCKeyboardDispatcher.hpp>

#if defined(GEODE_IS_IOS)
    #pragma pop_macro("GEODE_APPLY_MODIFY_FOR_CONSTRUCTOR")
    #pragma pop_macro("GEODE_APPLY_MODIFY_FOR_DESTRUCTOR")
#endif
