#pragma once
#include <Geode/Geode.hpp>
#include <algorithm>
#include <string>

// Pengganti fungsi GD yang tidak punya alamat di bindings macOS 2.2091.
// Di platform lain, kode asli tetap dipakai.
namespace eclipse::utils::compat {

    /// Pengganti cocos2d::ZipUtils::base64URLDecode (tidak ada alamat di macOS).
    inline std::string base64UrlDecode(std::string const& in) {
        auto val = [](char c) -> int {
            if (c >= 'A' && c <= 'Z') return c - 'A';
            if (c >= 'a' && c <= 'z') return c - 'a' + 26;
            if (c >= '0' && c <= '9') return c - '0' + 52;
            if (c == '+' || c == '-') return 62;
            if (c == '/' || c == '_') return 63;
            return -1;
        };
        std::string out;
        out.reserve(in.size() * 3 / 4);
        int buf = 0, bits = 0;
        for (char c : in) {
            if (c == '=') break;
            int v = val(c);
            if (v < 0) continue;
            buf = (buf << 6) | v;
            bits += 6;
            if (bits >= 8) {
                bits -= 8;
                out.push_back(static_cast<char>((buf >> bits) & 0xFF));
            }
        }
        return out;
    }

    /// Pengganti CCMenu::alignItemsHorizontallyWithPadding (tidak ada alamat di macOS).
    inline void alignHorizontally(cocos2d::CCMenu* menu, float padding) {
        auto* children = menu->getChildren();
        if (!children || children->count() == 0) return;
        float total = 0.f;
        for (unsigned i = 0; i < children->count(); ++i) {
            auto* n = static_cast<cocos2d::CCNode*>(children->objectAtIndex(i));
            total += n->getContentSize().width * n->getScaleX();
        }
        total += padding * (children->count() - 1);
        float x = -total / 2.f;
        for (unsigned i = 0; i < children->count(); ++i) {
            auto* n = static_cast<cocos2d::CCNode*>(children->objectAtIndex(i));
            float w = n->getContentSize().width * n->getScaleX();
            n->setPosition({x + w / 2.f, 0.f});
            x += w + padding;
        }
    }

    /// Pengganti CCTextInputNode::setAllowedChars (tidak ada alamat di macOS).
    template <typename Node>
    void setAllowedChars(Node* node, std::string const& chars) {
#if defined(__APPLE__)
        if constexpr (requires { node->m_allowedChars = chars; }) {
            node->m_allowedChars = chars;
            return;
        }
#endif
        node->setAllowedChars(chars);
    }

    /// Pengganti GameStatsManager::hasSecretCoin (tidak ada alamat di macOS).
    template <typename Gsm>
    bool hasSecretCoin(Gsm* gsm, char const* key) {
#if defined(__APPLE__)
        if constexpr (requires { gsm->m_secretCoins->objectForKey(key); }) {
            return gsm->m_secretCoins->objectForKey(key) != nullptr;
        } else {
            return false;
        }
#else
        return gsm->hasSecretCoin(key);
#endif
    }
}
