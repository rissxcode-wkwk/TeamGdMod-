#pragma once

#include <optional>
#include <type_traits>
#include <utility>

namespace eclipse::utils {
    /// Pengganti `opt.value_or({})`. Pada std::optional, `value_or({})` gagal dikompilasi karena
    /// tipe template tidak bisa disimpulkan dari `{}` (terjadi sejak asp::fs di Geode 6 mengembalikan std::optional).
    template <typename T>
    constexpr auto orEmpty(T&& opt) {
        using Value = typename std::remove_cvref_t<T>::value_type;
        return std::forward<T>(opt).value_or(Value{});
    }

    /// GJBaseGameLayer::m_allowedButtons tidak ada di bindings GD 2.2091. Dikosongkan hanya kalau member-nya ada.
    template <typename Layer>
    constexpr void clearAllowedButtons(Layer* layer) {
        if constexpr (requires { layer->m_allowedButtons.clear(); }) {
            layer->m_allowedButtons.clear();
        }
    }
}
