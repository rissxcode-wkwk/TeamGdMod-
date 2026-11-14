#pragma once
// Shim khusus iOS (untuk semua platform lain tidak melakukan apa-apa).
// Bindings GD 2.2091 belum punya alamat iOS untuk sebagian fungsi/ctor/dtor. Tanpa shim ini header
// modify bawaan gagal ("does not have an available address" / "expected expression").
// Di iOS, hook untuk fungsi tanpa alamat dilewati (fitur terkait tidak aktif di iOS).
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

#include_next <Geode/modify/CustomListView.hpp>

#if defined(GEODE_IS_IOS)
    #pragma pop_macro("GEODE_APPLY_MODIFY_FOR_FUNCTION_ERROR_DEFINED")
    #pragma pop_macro("GEODE_APPLY_MODIFY_FOR_CONSTRUCTOR")
    #pragma pop_macro("GEODE_APPLY_MODIFY_FOR_DESTRUCTOR")
#endif
