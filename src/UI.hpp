#pragma once
#include "Features.hpp"
#include "Bot.hpp"
#include "Labels.hpp"

namespace tg {

// aksi yang butuh akses ke PlayLayer (didefinisikan di main.cpp)
void actRestart();
void actPractice();
void actPlaceCheckpoint();
void actRemoveCheckpoint();
void actShowOptions();
void actJump(bool p1);

// ------------------------------------------------------------------ tema & gambar
struct Theme {
    ccColor3B accent{150, 40, 190}, bg{18, 18, 26};
    float op = 0.92f, bw = 1.f, radius = 5.f, fsz = 1.f, base = 0.3f;
    int style = 0;
    std::string font = "bigFont.fnt";
};

inline Theme theme() {
    Theme t;
    t.accent = color("interface-accent-color");
    t.bg = color("interface-background-color");
    t.op = get("interface-panel-opacity");
    t.bw = get("interface-border-width");
    t.radius = get("interface-corner-radius");
    t.fsz = get("interface-font-size");
    t.style = (int)get("interface-style");
    static std::vector<std::string> fonts = {"bigFont", "goldFont", "chatFont", "gjFont01", "gjFont02"};
    auto fi = std::clamp((int)get("interface-font"), 0, (int)fonts.size() - 1);
    t.font = fonts[fi] + ".fnt";
    t.base = fonts[fi] == "chatFont" ? 0.48f : 0.3f;
    if (t.style == 1) { t.radius = 0.f; t.bw = 0.f; }
    return t;
}

inline ccColor4F c4f(ccColor3B c, float a = 1.f) { return ccc4f(c.r / 255.f, c.g / 255.f, c.b / 255.f, a); }
inline ccColor3B mix(ccColor3B a, ccColor3B b, float t) {
    return {(GLubyte)(a.r + (b.r - a.r) * t), (GLubyte)(a.g + (b.g - a.g) * t), (GLubyte)(a.b + (b.b - a.b) * t)};
}

inline CCDrawNode* rrect(CCSize sz, float r, ccColor4F fill, float bw, ccColor4F border) {
    auto d = CCDrawNode::create();
    r = std::max(0.f, std::min(r, std::min(sz.width, sz.height) / 2.f));
    std::vector<CCPoint> p;
    auto arc = [&](float cx, float cy, float a0) {
        int n = r < 0.5f ? 1 : 6;
        for (int i = 0; i <= n; ++i) {
            float a = (a0 + (r < 0.5f ? 0.f : i * 15.f)) * 3.14159265f / 180.f;
            p.push_back({cx + std::cos(a) * r, cy + std::sin(a) * r});
        }
    };
    arc(sz.width - r, sz.height - r, 0);
    arc(r, sz.height - r, 90);
    arc(r, r, 180);
    arc(sz.width - r, r, 270);
    d->drawPolygon(p.data(), (unsigned)p.size(), fill, bw, border);
    return d;
}

// ------------------------------------------------------------------ menu
class TGMenu : public CCLayer {
public:
    static inline TGMenu* s_open = nullptr;

private:
    struct Hit {
        CCNode* node = nullptr;
        CCSize size{};
        CCNode* clip = nullptr;
        bool inRow = false;
        bool press = true;
        std::function<void()> click;
        std::function<void(CCPoint)> drag;
        std::function<void()> end;
    };
    struct Overlay {
        CCNode* node = nullptr;
        CCSize size{};
        std::vector<Hit> hits;
        std::function<void()> outside;
        int kind = 0; // 1 = dropdown, 2 = palette, 3 = keyboard
    };
    struct Panel {
        std::string cat;
        CCNode* node = nullptr;
        CCSize size{};
        CCNode* content = nullptr;
        float viewH = 0, contentH = 0;
        CCLayerColor* bar = nullptr;
    };
    enum class Mode { None, Window, Press, Scroll, Slider };

    static constexpr float HDR = 22.f, PAD = 6.f, PW = 150.f, GAP = 6.f, HP = 20.f;

    CCNode* m_win = nullptr;
    CCNode* m_hview = nullptr;
    CCNode* m_row = nullptr;
    CCLayerColor* m_hbar = nullptr;
    CCSize m_size{};
    Theme m_th;
    std::string m_fnt;
    float m_base = 0.3f, m_rowW = 0, m_viewW = 0, m_hoff = 0, m_scale = 1.f;
    std::vector<Hit> m_hits;
    std::vector<Overlay> m_ov;
    std::vector<Panel> m_panels;
    std::map<std::string, float> m_voff;
    std::set<std::string> m_expanded;
    std::string m_tab = "Global", m_focus;
    bool m_dirty = false, m_minimized = false, m_closing = false, m_hasActive = false;
    Mode m_mode = Mode::None;
    Hit m_active;
    CCPoint m_start{}, m_last{};
    int m_axis = 0, m_scrollPanel = -1;

    // keyboard / palette state
    std::string m_kbBuf, m_query;
    CCLabelBMFont* m_kbLabel = nullptr;
    bool m_kbNumeric = false;
    std::function<void(std::string const&)> m_kbChange;
    std::function<void(std::string const&, bool)> m_kbDone;
    int m_palTab = 0, m_palPage = 0;

    template <class T> static void enableExtras(T* t) {
        if constexpr (requires { t->setMouseEnabled(true); }) t->setMouseEnabled(true);
        if constexpr (requires { t->setKeyboardEnabled(true); }) t->setKeyboardEnabled(true);
        if constexpr (requires { t->setKeypadEnabled(true); }) t->setKeypadEnabled(true);
    }

    bool init() override {
        if (!CCLayer::init()) return false;
        auto dim = CCLayerColor::create({0, 0, 0, 105});
        addChild(dim, -1);
        setTouchMode(kCCTouchesOneByOne);
        setTouchPriority(-500);
        setTouchEnabled(true);
        enableExtras(this);
        m_minimized = Mod::get()->getSavedValue<bool>("menu-minimized", false);
        m_tab = Mod::get()->getSavedValue<std::string>("tab", "Global");
        build();
        if (on("interface-animations") && m_win) {
            float s = m_scale;
            m_win->setScale(s * 0.85f);
            m_win->runAction(CCEaseExponentialOut::create(CCScaleTo::create(0.2f / std::max(0.3f, get("interface-animation-speed")), s)));
        }
        return true;
    }

public:
    ~TGMenu() override {
        if (s_open == this) s_open = nullptr;
        g_menuOpen = s_open != nullptr;
    }

    static void toggle() {
        if (s_open) { s_open->closeMenu(); return; }
        auto sc = CCScene::get();
        if (!sc) return;
        auto m = new TGMenu();
        if (m && m->init()) {
            m->autorelease();
            s_open = m;
            g_menuOpen = true;
            sc->addChild(m, 9999);
        } else {
            CC_SAFE_DELETE(m);
        }
    }
    static void openPalette() {
        if (!s_open) toggle();
        if (s_open) s_open->showPalette();
    }

    void closeNow() {
        if (m_closing) return;
        m_closing = true;
        if (on("config-auto-save")) { saveConfig(); (void)Mod::get()->saveData(); }
        s_open = nullptr;
        g_menuOpen = false;
        setTouchEnabled(false);
        this->retain();
        this->autorelease();
        this->removeFromParent();
    }
    void closeMenu() {
        if (m_closing) return;
        if (on("interface-animations") && m_win) {
            m_closing = true;
            if (on("config-auto-save")) { saveConfig(); (void)Mod::get()->saveData(); }
            g_menuOpen = false;
            m_win->runAction(CCSequence::create(
                CCScaleTo::create(0.12f / std::max(0.3f, get("interface-animation-speed")), m_scale * 0.85f),
                CCCallFunc::create(this, callfunc_selector(TGMenu::finishClose)), nullptr));
        } else {
            closeNow();
        }
    }
    void finishClose() {
        m_closing = false;
        closeNow();
    }
    void keyBackClicked() override { closeMenu(); }

private:
    // ---------------------------------------------------------------- helpers
    static bool inside(CCNode* n, CCSize s, CCPoint w) {
        auto p = n->convertToNodeSpace(w);
        return p.x >= 0 && p.y >= 0 && p.x <= s.width && p.y <= s.height;
    }
    bool hitValid(Hit const& h, CCPoint w) const {
        if (!inside(h.node, h.size, w)) return false;
        if (h.clip && !inside(h.clip, h.clip->getContentSize(), w)) return false;
        if (h.inRow && m_hview && !inside(m_hview, m_hview->getContentSize(), w)) return false;
        return true;
    }
    Hit* pick(std::vector<Hit>& v, CCPoint w) {
        for (auto it = v.rbegin(); it != v.rend(); ++it) if (hitValid(*it, w)) return &*it;
        return nullptr;
    }

    CCNode* clipNode(CCSize sz) {
        auto st = CCDrawNode::create();
        CCPoint r[4] = {{0, 0}, {sz.width, 0}, {sz.width, sz.height}, {0, sz.height}};
        st->drawPolygon(r, 4, ccc4f(1, 1, 1, 1), 0, ccc4f(1, 1, 1, 1));
        auto cn = CCClippingNode::create(st);
        cn->setContentSize(sz);
        return cn;
    }

    CCLabelBMFont* mkLabel(std::string const& s, float sc, ccColor3B col, GLubyte op = 255) {
        auto l = CCLabelBMFont::create(s.c_str(), m_fnt.c_str());
        l->setScale(sc * m_base);
        l->setColor(col);
        l->setOpacity(op);
        return l;
    }
    static void fit(CCLabelBMFont* l, float maxW) {
        float w = l->getContentWidth() * l->getScale();
        if (w > maxW && w > 0) l->setScale(l->getScale() * maxW / w);
    }
    CCLabelBMFont* placeLabel(CCNode* parent, std::string const& s, CCPoint pos, CCPoint anchor, float maxW,
                              ccColor3B col = {235, 235, 240}, GLubyte op = 255, float sc = 1.f) {
        auto l = mkLabel(s, sc, col, op);
        l->setAnchorPoint(anchor);
        l->setPosition(pos);
        fit(l, maxW);
        parent->addChild(l, 2);
        return l;
    }
    static std::string fmtv(float v, bool integer) { return integer ? fmt::format("{:.0f}", v) : fmt::format("{:.2f}", v); }

    ccColor3B panelColor() const { return mix(m_th.bg, {255, 255, 255}, 0.07f); }
    ccColor3B btnColor() const { return mix(m_th.bg, {255, 255, 255}, 0.16f); }

    CCNode* region(CCNode* parent, CCPoint pos, CCSize sz) {
        auto n = CCNode::create();
        n->setPosition(pos);
        n->setContentSize(sz);
        parent->addChild(n);
        return n;
    }
    void addHit(CCNode* n, CCSize sz, std::function<void()> cb, CCNode* clip, bool inRow, bool press = true) {
        Hit h;
        h.node = n; h.size = sz; h.clip = clip; h.inRow = inRow; h.click = std::move(cb); h.press = press;
        m_hits.push_back(std::move(h));
    }

    CCNode* addButton(CCNode* parent, std::string const& text, CCPoint pos, CCSize sz, std::function<void()> cb,
                      CCNode* clip, bool inRow, bool filled = false, bool ready = true) {
        auto n = region(parent, pos, sz);
        n->setAnchorPoint({0.5f, 0.5f});
        n->setPosition(pos + CCPoint{sz.width / 2, sz.height / 2});
        float a = ready ? 1.f : 0.3f;
        auto base = filled ? m_th.accent : btnColor();
        n->addChild(rrect(sz, std::min(m_th.radius, 6.f), c4f(base, a), 0, c4f(base, a)));
        auto l = mkLabel(text, 1.f, ready ? ccColor3B{240, 240, 245} : ccColor3B{170, 170, 175}, ready ? 255 : 90);
        l->setPosition({sz.width / 2, sz.height / 2});
        fit(l, sz.width - 4.f);
        n->addChild(l, 2);
        if (ready && cb) addHit(n, sz, std::move(cb), clip, inRow);
        return n;
    }

    // ---------------------------------------------------------------- layout
    void applyWin() {
        auto win = CCDirector::get()->getWinSize();
        float s = std::min({get("interface-ui-scale"), (win.width - 4.f) / m_size.width, (win.height - 4.f) / m_size.height});
        m_scale = s;
        float hw = m_size.width * s / 2, hh = m_size.height * s / 2;
        float fx = (float)Mod::get()->getSavedValue<double>("menu-fx", 0.5);
        float fy = (float)Mod::get()->getSavedValue<double>("menu-fy", 0.5);
        m_win->setPosition({std::clamp(fx * win.width, hw, std::max(hw, win.width - hw)),
                            std::clamp(fy * win.height, hh, std::max(hh, win.height - hh))});
        m_win->setScale(s);
    }

    static float rowHeight(Feature const& f, bool expanded) {
        switch (f.kind) {
            case Kind::Slider: return 32.f;
            case Kind::Color: return 22.f + (expanded ? 96.f : 0.f);
            case Kind::Dropdown: return 22.f;
            case Kind::Action: return 22.f;
            default: return 20.f;
        }
    }

    void build() {
        m_hits.clear(); m_ov.clear(); m_panels.clear();
        m_hview = nullptr; m_row = nullptr; m_hbar = nullptr; m_kbLabel = nullptr;
        if (m_win) m_win->removeFromParent();
        m_th = theme();
        m_fnt = m_th.font;
        m_base = m_th.base * m_th.fsz;

        auto win = CCDirector::get()->getWinSize();
        float W = std::min(win.width * 0.86f, 800.f), H = std::min(win.height * 0.84f, 300.f);
        if (m_minimized) H = HDR + 2.f;
        m_size = CCSize{W, H};
        m_win = CCNode::create();
        m_win->setContentSize(m_size);
        m_win->setAnchorPoint({0.5f, 0.5f});
        addChild(m_win, 1);
        applyWin();

        float fillA = m_th.style == 2 ? m_th.op * 0.35f : m_th.op;
        float bw = m_th.style == 2 ? std::max(2.f, m_th.bw) : m_th.bw;
        m_win->addChild(rrect(m_size, m_th.radius, c4f(m_th.bg, fillA), bw, c4f(m_th.accent, 1.f)), -1);

        // header
        auto hd = rrect({W - 4.f, HDR - 2.f}, std::min(m_th.radius, 6.f), c4f(m_th.accent, 1.f), 0, c4f(m_th.accent, 1.f));
        hd->setPosition({2.f, H - HDR});
        m_win->addChild(hd, 0);
        placeLabel(m_win, "TeamGdMod v1.0.0", {10.f, H - HDR / 2 - 1.f}, {0, 0.5f}, 150.f, {255, 255, 255}, 255, 1.15f);

        float bx = W - 6.f;
        auto hbtn = [&](std::string txt, float w, std::function<void()> cb) {
            bx -= w;
            addButton(m_win, txt, {bx, H - HDR + 2.f}, {w, HDR - 6.f}, std::move(cb), nullptr, false);
            bx -= 3.f;
        };
        hbtn("X", 16.f, [this] { closeMenu(); });
        hbtn(m_minimized ? "+" : "-", 16.f, [this] {
            m_minimized = !m_minimized;
            Mod::get()->setSavedValue<bool>("menu-minimized", m_minimized);
            m_dirty = true;
        });
        hbtn("Layout", 36.f, [this] {
            for (auto& c : categories()) Mod::get()->setSavedValue<bool>("collapsed:" + c, false);
            m_voff.clear(); m_hoff = 0; m_focus.clear(); m_expanded.clear();
            m_dirty = true;
        });
        hbtn("Pos", 24.f, [this] {
            Mod::get()->setSavedValue<double>("menu-fx", 0.5);
            Mod::get()->setSavedValue<double>("menu-fy", 0.5);
            m_dirty = true;
        });
        hbtn("Palette", 42.f, [this] { showPalette(); });
        if (m_minimized) return;

        float bx0 = PAD, by0 = PAD, bW = W - 2 * PAD, bH = H - HDR - 2 * PAD;
        auto const& cats = categories();
        bool tabbed = (int)get("interface-layout-type") == 1;
        if (std::find(cats.begin(), cats.end(), m_tab) == cats.end()) m_tab = cats[0];

        if (tabbed) {
            float th = 18.f, tw = bW / cats.size();
            for (size_t i = 0; i < cats.size(); ++i) {
                auto c = cats[i];
                addButton(m_win, c, {bx0 + i * tw, by0 + bH - th}, {tw - 2.f, th - 2.f}, [this, c] {
                    m_tab = c;
                    Mod::get()->setSavedValue<std::string>("tab", c);
                    m_dirty = true;
                }, nullptr, false, c == m_tab);
            }
            auto holder = region(m_win, {bx0, by0}, {bW, bH - th - 2.f});
            makePanel(holder, m_tab, 0.f, bW, bH - th - 2.f);
        } else {
            m_viewW = bW;
            m_hview = clipNode({bW, bH});
            m_hview->setPosition({bx0, by0});
            m_win->addChild(m_hview, 1);
            m_row = CCNode::create();
            m_hview->addChild(m_row);
            m_rowW = cats.size() * (PW + GAP) - GAP;
            m_hoff = std::clamp(m_hoff, 0.f, std::max(0.f, m_rowW - bW));
            m_row->setPositionX(-m_hoff);
            for (size_t i = 0; i < cats.size(); ++i) makePanel(m_row, cats[i], i * (PW + GAP), PW, bH - 6.f);
            m_hbar = CCLayerColor::create({m_th.accent.r, m_th.accent.g, m_th.accent.b, 170}, 20.f, 3.f);
            m_hbar->setPosition({0.f, 0.f});
            m_hview->addChild(m_hbar, 5);
            updateBars();
        }
    }

    void makePanel(CCNode* parent, std::string const& cat, float x, float w, float h) {
        bool collapsed = Mod::get()->getSavedValue<bool>("collapsed:" + cat, false);
        bool inRow = m_hview != nullptr;
        auto node = region(parent, {x, inRow ? 6.f : 0.f}, {w, h});
        float ph = collapsed ? HP : h;
        float bw = m_th.style == 2 ? std::max(1.f, m_th.bw) : std::min(m_th.bw, 1.f);
        auto bg = rrect({w, ph}, std::min(m_th.radius, 8.f), c4f(panelColor(), m_th.style == 2 ? 0.3f : 0.85f), bw,
                        c4f(mix(m_th.bg, m_th.accent, 0.55f), 1.f));
        bg->setPosition({0, h - ph});
        node->addChild(bg, 0);
        auto hdr = rrect({w, HP}, std::min(m_th.radius, 6.f), c4f(m_th.accent, 1.f), 0, c4f(m_th.accent, 1.f));
        hdr->setPosition({0, h - HP});
        node->addChild(hdr, 1);
        auto hn = region(node, {0, h - HP}, {w, HP});
        placeLabel(hn, std::string(collapsed ? "> " : "v ") + cat, {6.f, HP / 2}, {0, 0.5f}, w - 10.f, {255, 255, 255}, 255, 1.05f);
        addHit(hn, {w, HP}, [this, cat, collapsed] {
            Mod::get()->setSavedValue<bool>("collapsed:" + cat, !collapsed);
            m_dirty = true;
        }, nullptr, inRow, false);

        Panel p;
        p.cat = cat; p.node = node; p.size = CCSize{w, h};
        if (collapsed) { m_panels.push_back(p); return; }

        float viewH = h - HP - 6.f, cw = w - 8.f;
        auto view = clipNode({cw, viewH});
        view->setPosition({4.f, 3.f});
        node->addChild(view, 2);
        auto content = CCNode::create();
        view->addChild(content);

        float cursor = 4.f;
        for (auto& f : features()) {
            if (f.cat != cat) continue;
            cursor += buildRow(content, view, f, cw, -cursor);
        }
        float contentH = cursor + 2.f;
        float rng = std::max(0.f, contentH - viewH);
        float& off = m_voff[cat];
        off = std::clamp(off, 0.f, rng);
        content->setPositionY(viewH + off);

        p.content = content; p.viewH = viewH; p.contentH = contentH;
        p.bar = CCLayerColor::create({m_th.accent.r, m_th.accent.g, m_th.accent.b, 170}, 2.f, 10.f);
        p.bar->setPosition({w - 4.f, 4.f});
        node->addChild(p.bar, 6);
        m_panels.push_back(p);
        updateBars();
    }

    void updateBars() {
        for (auto& p : m_panels) {
            if (!p.bar || !p.content) continue;
            float rng = p.contentH - p.viewH;
            p.bar->setVisible(rng > 0.f);
            if (rng <= 0.f) continue;
            float bh = std::max(10.f, p.viewH * p.viewH / p.contentH);
            float t = std::clamp(m_voff[p.cat] / rng, 0.f, 1.f);
            p.bar->setContentSize({2.f, bh});
            p.bar->setPosition({p.size.width - 4.f, 3.f + p.viewH - bh - t * (p.viewH - bh)});
        }
        if (m_hbar && m_row) {
            float rng = m_rowW - m_viewW;
            m_hbar->setVisible(rng > 0.f);
            if (rng > 0.f) {
                float bw = std::max(20.f, m_viewW * m_viewW / m_rowW);
                m_hbar->setContentSize({bw, 3.f});
                m_hbar->setPosition({std::clamp(m_hoff / rng, 0.f, 1.f) * (m_viewW - bw), 0.f});
            }
        }
    }

    void scrollPanel(int i, float dy) {
        if (i < 0 || i >= (int)m_panels.size()) return;
        auto& p = m_panels[i];
        if (!p.content) return;
        float rng = std::max(0.f, p.contentH - p.viewH);
        float& off = m_voff[p.cat];
        off = std::clamp(off + dy, 0.f, rng);
        p.content->setPositionY(p.viewH + off);
        updateBars();
    }
    void scrollRow(float dx) {
        if (!m_row) return;
        m_hoff = std::clamp(m_hoff + dx, 0.f, std::max(0.f, m_rowW - m_viewW));
        m_row->setPositionX(-m_hoff);
        updateBars();
    }
    int panelAt(CCPoint w) {
        for (size_t i = 0; i < m_panels.size(); ++i) {
            auto& p = m_panels[i];
            if (!inside(p.node, p.size, w)) continue;
            if (m_hview && !inside(m_hview, m_hview->getContentSize(), w)) continue;
            return (int)i;
        }
        return -1;
    }

    // ---------------------------------------------------------------- baris kontrol
    void drawBox(CCDrawNode* d, bool v, float sz, float a) {
        d->clear();
        CCPoint r[4] = {{0, 0}, {sz, 0}, {sz, sz}, {0, sz}};
        d->drawPolygon(r, 4, c4f(v ? m_th.accent : btnColor(), a), 1.f, c4f(mix(m_th.bg, {255, 255, 255}, 0.4f), a));
        if (v) {
            d->drawSegment({sz * 0.22f, sz * 0.5f}, {sz * 0.43f, sz * 0.25f}, 0.9f, ccc4f(1, 1, 1, a));
            d->drawSegment({sz * 0.43f, sz * 0.25f}, {sz * 0.80f, sz * 0.75f}, 0.9f, ccc4f(1, 1, 1, a));
        }
    }

    void sliderRow(CCNode* n, CCNode* view, float w, float h, std::string const& name, float mn, float mx, float cur,
                   bool ready, bool integer, std::function<void(float)> set) {
        float al = ready ? 1.f : 0.3f;
        GLubyte op = ready ? 255 : 90;
        placeLabel(n, name, {3.f, h - 8.f}, {0, 0.5f}, w * 0.6f, {235, 235, 240}, op);
        auto vl = placeLabel(n, fmtv(cur, integer), {w - 3.f, h - 8.f}, {1.f, 0.5f}, w * 0.38f, mix(m_th.accent, {255, 255, 255}, 0.45f), op);
        float tw = w - 8.f;
        auto tn = region(n, {4.f, 3.f}, {tw, 12.f});
        auto state = std::make_shared<float>(cur);
        auto tr = CCDrawNode::create();
        tn->addChild(tr);
        auto redraw = [=, this]() {
            tr->clear();
            float t = mx > mn ? std::clamp((*state - mn) / (mx - mn), 0.f, 1.f) : 0.f;
            tr->drawSegment({0, 6.f}, {tw, 6.f}, 1.6f, c4f(btnColor(), al));
            tr->drawSegment({0, 6.f}, {t * tw, 6.f}, 1.6f, c4f(m_th.accent, al));
            tr->drawDot({t * tw, 6.f}, 4.5f, ccc4f(1, 1, 1, al));
        };
        redraw();
        if (!ready) return;
        auto apply = [=, this](float v) {
            v = std::clamp(v, mn, mx);
            v = integer ? std::round(v) : std::round(v * 100.f) / 100.f;
            *state = v;
            set(v);
            vl->setString(fmtv(v, integer).c_str());
            redraw();
        };
        Hit h2;
        h2.node = tn; h2.size = CCSize{tw, 12.f}; h2.clip = view; h2.inRow = m_hview != nullptr; h2.press = false;
        h2.drag = [=](CCPoint p) { apply(mn + std::clamp(p.x / tw, 0.f, 1.f) * (mx - mn)); };
        h2.end = [this] { m_dirty = m_dirty || m_uiDirty; };
        m_hits.push_back(std::move(h2));
        // ketuk angka -> input angka dengan validasi
        auto vn = region(n, {w * 0.5f, h - 16.f}, {w * 0.5f - 2.f, 16.f});
        addHit(vn, vn->getContentSize(), [=, this] {
            openKeyboard(name, fmtv(*state, integer), true, nullptr, [=](std::string const& s, bool ok) {
                if (!ok || s.empty()) return;
                try { apply(std::stof(s)); } catch (...) {}
            });
        }, view, m_hview != nullptr, false);
    }

    bool m_uiDirty = false;

    float buildRow(CCNode* content, CCNode* view, Feature const& f, float w, float yTop) {
        bool expanded = m_expanded.count(f.id) > 0;
        float rh = rowHeight(f, expanded);
        auto n = CCNode::create();
        n->setPosition({0, yTop - rh});
        n->setContentSize({w, rh});
        content->addChild(n);
        bool rd = f.ready && !g_unsupported.count(f.id), inRow = m_hview != nullptr;
        float al = rd ? 1.f : 0.3f;
        GLubyte op = rd ? 255 : 90;
        if (f.id == m_focus)
            n->addChild(rrect({w, rh - 2.f}, 3.f, c4f(m_th.accent, 0.18f), 1.f, c4f(m_th.accent, 1.f)), -1);
        auto const* pf = &f;
        bool ui = f.cat == "Interface";

        switch (f.kind) {
            case Kind::Toggle: {
                float s = 11.f;
                auto box = CCDrawNode::create();
                box->setPosition({3.f, (rh - s) / 2});
                n->addChild(box);
                drawBox(box, on(f.id), s, al);
                placeLabel(n, f.name, {20.f, rh / 2}, {0, 0.5f}, w - 24.f, {235, 235, 240}, op);
                if (rd) addHit(n, {w, rh}, [=, this] {
                    setB(*pf, !on(pf->id));
                    drawBox(box, on(pf->id), s, 1.f);
                    if (ui) m_dirty = true;
                }, view, inRow, false);
                break;
            }
            case Kind::Slider: {
                bool integer = f.max >= 100.f;
                sliderRow(n, view, w, rh, f.name, f.min, f.max, get(f.id), rd, integer, [=, this](float v) {
                    setV(*pf, v);
                    if (pf->id == "interface-ui-scale") applyWin();
                    if (ui && pf->id != "interface-ui-scale") m_uiDirty = true;
                    if (pf->id == "global-fps") Mod::get()->setSavedValue<double>(skey(pf->id), v);
                });
                if (ui) m_uiDirty = false;
                break;
            }
            case Kind::Dropdown: {
                placeLabel(n, f.name, {3.f, rh / 2}, {0, 0.5f}, w * 0.42f, {235, 235, 240}, op);
                float bwid = w * 0.54f;
                auto opts = optionsFor(f);
                int cur = std::clamp((int)get(f.id), 0, std::max(0, (int)opts.size() - 1));
                auto bn = region(n, {w - bwid - 2.f, 2.f}, {bwid, rh - 4.f});
                bn->addChild(rrect(bn->getContentSize(), 4.f, c4f(btnColor(), al), 0, c4f(btnColor(), al)));
                placeLabel(bn, (opts.empty() ? std::string("-") : opts[cur]) + " v", {bwid / 2, (rh - 4.f) / 2}, {0.5f, 0.5f}, bwid - 6.f, {235, 235, 240}, op);
                if (rd) addHit(bn, bn->getContentSize(), [=, this] {
                    openDropdown(bn, bn->getContentSize(), optionsFor(*pf), cur, [=, this](int i) {
                        setV(*pf, (float)i);
                        if (pf->id == "interface-theme") {
                            auto tc = themeColors(i);
                            setColor(*find("interface-accent-color"), {(GLubyte)(tc.first >> 16), (GLubyte)((tc.first >> 8) & 255), (GLubyte)(tc.first & 255)});
                            setColor(*find("interface-background-color"), {(GLubyte)(tc.second >> 16), (GLubyte)((tc.second >> 8) & 255), (GLubyte)(tc.second & 255)});
                        }
                        m_dirty = true;
                    });
                }, view, inRow);
                break;
            }
            case Kind::Color: {
                placeLabel(n, f.name, {3.f, rh - 11.f}, {0, 0.5f}, w * 0.55f, {235, 235, 240}, op);
                auto sw = region(n, {w - 40.f, rh - 19.f}, {36.f, 16.f});
                sw->addChild(rrect({36.f, 16.f}, 4.f, c4f(color(f), al), 1.f, c4f({255, 255, 255}, 0.5f * al)));
                if (rd) addHit(sw, {36.f, 16.f}, [=, this] {
                    if (!m_expanded.erase(pf->id)) m_expanded.insert(pf->id);
                    m_dirty = true;
                }, view, inRow);
                if (expanded) {
                    char const* ch[3] = {"R", "G", "B"};
                    for (int i = 0; i < 3; ++i) {
                        auto sub = region(n, {0, rh - 22.f - (i + 1) * 32.f}, {w, 32.f});
                        sliderRow(sub, view, w, 32.f, ch[i], 0.f, 255.f, get(f.id + "#" + std::to_string(i)), rd, true,
                                  [=, this](float v) {
                                      setRaw(pf->id + "#" + std::to_string(i), v);
                                      m_uiDirty = true;
                                  });
                    }
                }
                break;
            }
            case Kind::Action: {
                addButton(n, f.name, {2.f, 2.f}, {w - 4.f, rh - 4.f}, [=, this] { runAction(pf->id); }, view, inRow, false, rd);
                break;
            }
        }
        return rh;
    }

    std::vector<std::string> optionsFor(Feature const& f) {
        if (f.id == "bot-replay-selection") {
            auto l = bot::list();
            if (l.empty()) l.push_back("(kosong)");
            return l;
        }
        return f.opts;
    }

    // ---------------------------------------------------------------- overlay umum
    int pushOverlay(CCNode* node, CCSize sz, std::function<void()> outside, int kind) {
        m_win->addChild(node, 100 + (int)m_ov.size() * 10);
        Overlay o;
        o.node = node; o.size = sz; o.outside = std::move(outside); o.kind = kind;
        m_ov.push_back(std::move(o));
        return (int)m_ov.size() - 1;
    }
    int overlayIndex(int kind) {
        for (size_t i = 0; i < m_ov.size(); ++i) if (m_ov[i].kind == kind) return (int)i;
        return -1;
    }
    void removeOverlay(int kind) {
        int i = overlayIndex(kind);
        if (i < 0) return;
        m_ov[i].node->removeFromParent();
        m_ov.erase(m_ov.begin() + i);
    }
    void ovHit(int kind, CCNode* n, CCSize sz, std::function<void()> cb) {
        int i = overlayIndex(kind);
        if (i < 0) return;
        Hit h;
        h.node = n; h.size = sz; h.click = std::move(cb);
        m_ov[i].hits.push_back(std::move(h));
    }

    void openDropdown(CCNode* anchor, CCSize asz, std::vector<std::string> opts, int cur, std::function<void(int)> sel) {
        removeOverlay(1);
        auto pos = m_win->convertToNodeSpace(anchor->convertToWorldSpace({0, 0}));
        float rowH = 16.f, h = opts.size() * rowH + 4.f, w = std::max(asz.width, 70.f);
        float y = pos.y - h;
        if (y < 4.f) y = std::min(pos.y + asz.height, m_size.height - h - 2.f);
        float x = std::clamp(pos.x, 2.f, m_size.width - w - 2.f);
        auto node = CCNode::create();
        node->setPosition({x, y});
        node->setContentSize({w, h});
        node->addChild(rrect({w, h}, 4.f, c4f(mix(m_th.bg, {255, 255, 255}, 0.1f), 1.f), 1.f, c4f(m_th.accent, 1.f)));
        pushOverlay(node, {w, h}, [this] { removeOverlay(1); }, 1);
        for (size_t i = 0; i < opts.size(); ++i) {
            auto rn = region(node, {2.f, h - 2.f - (i + 1) * rowH}, {w - 4.f, rowH});
            if ((int)i == cur) rn->addChild(rrect(rn->getContentSize(), 3.f, c4f(m_th.accent, 0.6f), 0, c4f(m_th.accent, 0.6f)));
            placeLabel(rn, opts[i], {5.f, rowH / 2}, {0, 0.5f}, w - 12.f);
            ovHit(1, rn, rn->getContentSize(), [this, sel, i] {
                removeOverlay(1);
                sel((int)i);
            });
        }
    }

    // ---------------------------------------------------------------- keyboard layar
    void kbRefreshLabel() {
        if (!m_kbLabel) return;
        m_kbLabel->setString((m_kbBuf + "|").c_str());
        fit(m_kbLabel, m_size.width - 60.f);
    }
    void kbType(char c) {
        if (m_kbBuf.size() >= 32) return;
        m_kbBuf += c;
        kbRefreshLabel();
        if (m_kbChange) m_kbChange(m_kbBuf);
    }
    void kbBack() {
        if (!m_kbBuf.empty()) m_kbBuf.pop_back();
        kbRefreshLabel();
        if (m_kbChange) m_kbChange(m_kbBuf);
    }
    void kbFinish(bool ok) {
        auto done = m_kbDone;
        auto buf = m_kbBuf;
        m_kbLabel = nullptr;
        removeOverlay(3);
        m_kbChange = nullptr;
        m_kbDone = nullptr;
        if (done) done(buf, ok);
        refreshPalette();
    }

    void openKeyboard(std::string const& title, std::string const& init, bool numeric,
                      std::function<void(std::string const&)> onChange,
                      std::function<void(std::string const&, bool)> onDone) {
        removeOverlay(3);
        m_kbBuf = init; m_kbNumeric = numeric;
        m_kbChange = std::move(onChange);
        m_kbDone = std::move(onDone);
        float W = m_size.width - 16.f;
        float kh = std::min(17.f, (m_size.height - HDR - 40.f) / 6.f);
        int rows = numeric ? 3 : 5;
        float H = rows * kh + 18.f + 6.f;
        auto node = CCNode::create();
        node->setPosition({8.f, 4.f});
        node->setContentSize({W, H});
        node->addChild(rrect({W, H}, 5.f, c4f(mix(m_th.bg, {255, 255, 255}, 0.08f), 1.f), 1.f, c4f(m_th.accent, 1.f)));
        pushOverlay(node, {W, H}, nullptr, 3);
        placeLabel(node, title + ":", {6.f, H - 9.f}, {0, 0.5f}, 90.f, mix(m_th.accent, {255, 255, 255}, 0.5f));
        m_kbLabel = placeLabel(node, (m_kbBuf + "|"), {96.f, H - 9.f}, {0, 0.5f}, W - 104.f);
        float kw = W / 11.f;
        auto key = [&](std::string const& t, int row, float x0, float wk, std::function<void()> cb, bool filled = false) {
            float y = H - 18.f - (row + 1) * kh;
            auto kn = region(node, {x0 + 1.f, y + 1.f}, {wk - 2.f, kh - 2.f});
            auto col = filled ? m_th.accent : btnColor();
            kn->addChild(rrect(kn->getContentSize(), 3.f, c4f(col, 1.f), 0, c4f(col, 1.f)));
            auto l = mkLabel(t, 1.f, {240, 240, 245});
            l->setPosition({(wk - 2.f) / 2, (kh - 2.f) / 2});
            fit(l, wk - 5.f);
            kn->addChild(l, 2);
            ovHit(3, kn, kn->getContentSize(), std::move(cb));
        };
        auto rowKeys = [&](std::string const& s, int row, float off) {
            for (size_t i = 0; i < s.size(); ++i) {
                char c = s[i];
                key(std::string(1, c), row, off + i * kw, kw, [this, c] { kbType(c); });
            }
        };
        rowKeys("1234567890", 0, kw / 2);
        if (numeric) {
            rowKeys(".-", 1, kw / 2);
            key("Back", 1, kw / 2 + 2 * kw, kw * 2, [this] { kbBack(); });
            key("Cancel", 2, kw / 2, kw * 3, [this] { kbFinish(false); });
            key("OK", 2, kw / 2 + 3 * kw, kw * 3, [this] { kbFinish(true); }, true);
        } else {
            rowKeys("qwertyuiop", 1, kw / 2);
            rowKeys("asdfghjkl", 2, kw);
            rowKeys("zxcvbnm._-", 3, kw / 2);
            key("Cancel", 4, kw / 2, kw * 2.5f, [this] { kbFinish(false); });
            key("Space", 4, kw / 2 + 2.5f * kw, kw * 4, [this] { kbType(' '); });
            key("Back", 4, kw / 2 + 6.5f * kw, kw * 1.5f, [this] { kbBack(); });
            key("OK", 4, kw / 2 + 8.f * kw, kw * 2, [this] { kbFinish(true); }, true);
        }
        kbRefreshLabel();
    }

    void handleKey(int k) {
        if (m_kbLabel) {
            if (k >= 65 && k <= 90) kbType((char)('a' + k - 65));
            else if (k >= 48 && k <= 57) kbType((char)('0' + k - 48));
            else if (k == 32) kbType(' ');
            else if (k == 8) kbBack();
            else if (k == 13) kbFinish(true);
            else if (k == 27) kbFinish(false);
        } else if (k == 27) {
            if (!m_ov.empty()) { auto o = m_ov.back().outside; if (o) o(); }
            else closeMenu();
        }
    }
public:
    // tanpa 'override' agar tetap compile bila signature berbeda antar versi
    void keyDown(enumKeyCodes key) { handleKey((int)key); }
    void keyDown(enumKeyCodes key, double) { handleKey((int)key); }

    void scrollWheel(float y, float x) {
        if (!m_ov.empty()) {
            int pi = overlayIndex(2);
            if (pi >= 0 && m_kbLabel == nullptr) { m_palPage = std::max(0, m_palPage + (y > 0 ? 1 : -1)); refreshPalette(); }
            return;
        }
        auto mp = geode::cocos::getMousePos();
        int i = panelAt(mp);
        if (std::abs(x) > 0.01f) scrollRow(x > 0 ? 24.f : -24.f);
        else if (i >= 0) scrollPanel(i, y > 0 ? 24.f : -24.f);
        else scrollRow(y > 0 ? 24.f : -24.f);
    }

private:
    // ---------------------------------------------------------------- command palette
    std::vector<Feature const*> palResults() {
        std::vector<Feature const*> src;
        if (m_palTab == 0) {
            for (auto& f : features()) src.push_back(&f);
            std::stable_partition(src.begin(), src.end(), [](Feature const* f) { return f->ready; });
        } else {
            for (auto& id : (m_palTab == 1 ? favorites() : history()))
                if (auto f = find(id)) src.push_back(f);
        }
        std::string q = m_query;
        for (auto& c : q) c = (char)std::tolower((unsigned char)c);
        std::vector<Feature const*> out;
        for (auto f : src) {
            std::string hay = f->name + " " + f->cat;
            for (auto& c : hay) c = (char)std::tolower((unsigned char)c);
            if (q.empty() || hay.find(q) != std::string::npos) out.push_back(f);
        }
        return out;
    }
    std::string stateText(Feature const& f) {
        if (!f.ready || g_unsupported.count(f.id)) return "-";
        switch (f.kind) {
            case Kind::Toggle: return on(f.id) ? "ON" : "OFF";
            case Kind::Slider: return fmtv(get(f.id), f.max >= 100.f);
            case Kind::Dropdown: {
                auto o = optionsFor(f);
                int i = (int)get(f.id);
                return i >= 0 && i < (int)o.size() ? o[i] : "-";
            }
            case Kind::Color: return "warna";
            default: return "RUN";
        }
    }

    void showPalette() {
        if (overlayIndex(2) >= 0) return;
        m_palTab = 0; m_palPage = 0;
        buildPalette();
    }
    void refreshPalette() {
        if (overlayIndex(2) < 0) return;
        // bangun ulang overlay palet di posisi yang sama
        int i = overlayIndex(2);
        m_ov[i].node->removeFromParent();
        m_ov.erase(m_ov.begin() + i);
        buildPalette(i);
    }
    void buildPalette(int atIndex = -1) {
        float W = m_size.width - 16.f, H = m_size.height - HDR - 8.f;
        auto node = CCNode::create();
        node->setPosition({8.f, 4.f});
        node->setContentSize({W, H});
        node->addChild(rrect({W, H}, 6.f, c4f(mix(m_th.bg, {255, 255, 255}, 0.05f), 0.98f), 1.f, c4f(m_th.accent, 1.f)));

        Overlay o;
        o.node = node; o.size = CCSize{W, H}; o.kind = 2;
        o.outside = [this] { removeOverlay(3); removeOverlay(2); };
        int z = 100;
        if (atIndex < 0 || atIndex > (int)m_ov.size()) atIndex = (int)m_ov.size();
        m_ov.insert(m_ov.begin() + atIndex, std::move(o));
        m_win->addChild(node, z + atIndex * 10);

        float y = H - 4.f;
        // baris pencarian
        auto sb = region(node, {6.f, y - 18.f}, {W - 12.f - 56.f, 16.f});
        sb->addChild(rrect(sb->getContentSize(), 4.f, c4f(btnColor(), 1.f), 1.f, c4f(m_th.accent, 0.8f)));
        placeLabel(sb, "Cari: " + m_query + (m_kbLabel ? "|" : ""), {6.f, 8.f}, {0, 0.5f}, W - 90.f);
        ovHit(2, sb, sb->getContentSize(), [this] {
            if (m_kbLabel) return;
            openKeyboard("Cari", m_query, false, [this](std::string const& s) {
                m_query = s; m_palPage = 0; refreshPalette();
            }, [this](std::string const&, bool) {});
        });
        auto xb = region(node, {W - 56.f, y - 18.f}, {50.f, 16.f});
        xb->addChild(rrect(xb->getContentSize(), 4.f, c4f(btnColor(), 1.f), 0, c4f(btnColor(), 1.f)));
        placeLabel(xb, "Tutup", {25.f, 8.f}, {0.5f, 0.5f}, 46.f);
        ovHit(2, xb, xb->getContentSize(), [this] { removeOverlay(3); removeOverlay(2); });
        y -= 22.f;
        // tab
        char const* tabs[3] = {"Semua", "Favorit", "Riwayat"};
        for (int t = 0; t < 3; ++t) {
            auto tn = region(node, {6.f + t * 60.f, y - 16.f}, {56.f, 15.f});
            auto col = t == m_palTab ? m_th.accent : btnColor();
            tn->addChild(rrect(tn->getContentSize(), 4.f, c4f(col, 1.f), 0, c4f(col, 1.f)));
            placeLabel(tn, tabs[t], {28.f, 7.5f}, {0.5f, 0.5f}, 52.f);
            ovHit(2, tn, tn->getContentSize(), [this, t] { m_palTab = t; m_palPage = 0; refreshPalette(); });
        }
        y -= 20.f;

        auto res = palResults();
        float rowH = 18.f;
        int perPage = std::max(2, (int)((y - 22.f - (m_kbLabel ? 112.f : 0.f)) / rowH));
        int pages = std::max(1, ((int)res.size() + perPage - 1) / perPage);
        m_palPage = std::clamp(m_palPage, 0, pages - 1);
        for (int i = 0; i < perPage; ++i) {
            int idx = m_palPage * perPage + i;
            if (idx >= (int)res.size()) break;
            auto f = res[idx];
            auto rn = region(node, {6.f, y - (i + 1) * rowH}, {W - 12.f, rowH - 1.f});
            bool fr = f->ready && !g_unsupported.count(f->id);
            rn->addChild(rrect(rn->getContentSize(), 3.f, c4f(btnColor(), fr ? 0.7f : 0.3f), 0, c4f(btnColor(), 0.f)));
            bool fav = isFavorite(f->id);
            placeLabel(rn, fav ? "*" : "+", {8.f, (rowH - 1.f) / 2}, {0.5f, 0.5f}, 12.f, fav ? ccColor3B{255, 210, 60} : ccColor3B{150, 150, 155}, 255, 1.3f);
            GLubyte op = fr ? 255 : 100;
            placeLabel(rn, f->name, {18.f, (rowH - 1.f) / 2}, {0, 0.5f}, (W - 12.f) * 0.5f, {235, 235, 240}, op);
            placeLabel(rn, f->cat, {18.f + (W - 12.f) * 0.5f + 4.f, (rowH - 1.f) / 2}, {0, 0.5f}, (W - 12.f) * 0.2f, {150, 150, 160}, op, 0.85f);
            bool isOn = f->kind == Kind::Toggle && f->ready && on(f->id);
            placeLabel(rn, stateText(*f), {W - 16.f, (rowH - 1.f) / 2}, {1.f, 0.5f}, (W - 12.f) * 0.2f,
                       isOn ? ccColor3B{90, 220, 120} : ccColor3B{200, 200, 205}, op);
            auto fstar = region(rn, {0, 0}, {16.f, rowH - 1.f});
            std::string id = f->id;
            ovHit(2, rn, rn->getContentSize(), [this, id] { runFeature(id); });
            // tombol bintang diutamakan (ditambahkan belakangan)
            ovHit(2, fstar, fstar->getContentSize(), [this, id] { toggleFavorite(id); refreshPalette(); });
        }
        if (res.empty()) placeLabel(node, "Tidak ada hasil", {W / 2, y - 20.f}, {0.5f, 0.5f}, W - 20.f, {160, 160, 165});
        // pager
        auto pb = [&](std::string t, float x, int d) {
            auto b = region(node, {x, 4.f}, {22.f, 15.f});
            b->addChild(rrect(b->getContentSize(), 4.f, c4f(btnColor(), 1.f), 0, c4f(btnColor(), 1.f)));
            placeLabel(b, t, {11.f, 7.5f}, {0.5f, 0.5f}, 20.f);
            ovHit(2, b, b->getContentSize(), [this, d] { m_palPage += d; refreshPalette(); });
        };
        if (!m_kbLabel) {
            pb("<", 6.f, -1);
            pb(">", 32.f, 1);
            placeLabel(node, fmt::format("{}/{}  ({} fitur)", m_palPage + 1, pages, res.size()), {60.f, 11.5f}, {0, 0.5f}, 120.f, {160, 160, 165});
            auto kb = region(node, {W - 44.f, 4.f}, {38.f, 15.f});
            kb->addChild(rrect(kb->getContentSize(), 4.f, c4f(m_th.accent, 1.f), 0, c4f(m_th.accent, 1.f)));
            placeLabel(kb, "Ketik", {19.f, 7.5f}, {0.5f, 0.5f}, 34.f);
            ovHit(2, kb, kb->getContentSize(), [this] {
                openKeyboard("Cari", m_query, false, [this](std::string const& s) {
                    m_query = s; m_palPage = 0; refreshPalette();
                }, [this](std::string const&, bool) {});
            });
        }
    }

    void runFeature(std::string const& id) {
        auto f = find(id);
        if (!f) return;
        bool fr = f->ready && !g_unsupported.count(f->id);
        if (fr && f->kind == Kind::Toggle) {
            setB(*f, !on(id));
            pushHistory(id);
            if (f->cat == "Interface") { m_dirty = true; return; }
            refreshPalette();
        } else if (fr && f->kind == Kind::Action) {
            pushHistory(id);
            runAction(id);
        } else {
            if (fr) pushHistory(id);
            navigateTo(id);
        }
    }

    void navigateTo(std::string const& id) {
        auto f = find(id);
        if (!f) return;
        removeOverlay(3);
        removeOverlay(2);
        removeOverlay(1);
        Mod::get()->setSavedValue<bool>("collapsed:" + f->cat, false);
        m_minimized = false;
        Mod::get()->setSavedValue<bool>("menu-minimized", false);
        m_tab = f->cat;
        float off = 4.f;
        for (auto& g : features()) {
            if (g.cat != f->cat) continue;
            if (g.id == id) break;
            off += rowHeight(g, m_expanded.count(g.id) > 0);
        }
        m_voff[f->cat] = std::max(0.f, off - 10.f);
        auto const& cats = categories();
        auto it = std::find(cats.begin(), cats.end(), f->cat);
        m_hoff = (float)std::distance(cats.begin(), it) * (PW + GAP);
        m_focus = id;
        m_dirty = true;
    }

    // ---------------------------------------------------------------- aksi
    static void notify(std::string const& s, bool ok = true) {
        Notification::create(s, ok ? NotificationIcon::Success : NotificationIcon::Error, 1.5f)->show();
    }

    void runAction(std::string const& id) {
        if (id == "shortcuts-restart-level") actRestart();
        else if (id == "shortcuts-toggle-practice-mode") actPractice();
        else if (id == "shortcuts-place-checkpoint") actPlaceCheckpoint();
        else if (id == "shortcuts-remove-checkpoint") actRemoveCheckpoint();
        else if (id == "shortcuts-step-frame") g_stepPending = true;
        else if (id == "shortcuts-p1-jump") actJump(true);
        else if (id == "shortcuts-p2-jump") actJump(false);
        else if (id == "shortcuts-show-options") { closeNow(); actShowOptions(); }
        else if (id == "keybinds-open-menu" || id == "keybinds-rebind-key" || id == "keybinds-clear-keybind" ||
                 id == "keybinds-reset-default") {
            closeNow();
            geode::openSettingsPopup(Mod::get(), false);
        }
        else if (id == "interface-reload-fonts") m_dirty = true;
        else if (id == "config-save-configuration") { bool ok = saveConfig(); notify(ok ? "Konfigurasi disimpan" : "Gagal menyimpan", ok); }
        else if (id == "config-load-configuration") {
            bool ok = loadConfig();
            notify(ok ? "Konfigurasi dimuat" : "Belum ada file konfigurasi", ok);
            m_dirty = true;
        }
        else if (id == "config-reset-configuration") {
            resetConfig();
            for (auto& c : categories()) Mod::get()->setSavedValue<bool>("collapsed:" + c, false);
            m_voff.clear(); m_hoff = 0; m_expanded.clear();
            notify("Konfigurasi direset");
            m_dirty = true;
        }
        else if (id == "bot-create-new-replay") { bot::rec.clear(); bot::frame = 0; bot::idx = 0; notify("Replay baru (kosong)"); }
        else if (id == "bot-save-replay") {
            openKeyboard("Nama replay", "replay", false, nullptr, [this](std::string const& s, bool ok) {
                if (!ok) return;
                bool r = bot::save(s);
                notify(r ? "Replay disimpan" : "Gagal menyimpan replay", r);
                m_dirty = true;
            });
        }
        else if (id == "bot-load-replay") {
            auto n = bot::selected();
            bool r = bot::load(n);
            notify(r ? "Replay dimuat: " + n : "Pilih replay dulu", r);
        }
        else if (id == "bot-delete-replay") {
            auto n = bot::selected();
            if (n.empty()) { notify("Pilih replay dulu", false); return; }
            openKeyboard("Ketik 'ya' untuk hapus " + n, "", false, nullptr, [this, n](std::string const& s, bool ok) {
                if (!ok || s != "ya") return;
                bool r = bot::remove(n);
                notify(r ? "Replay dihapus" : "Gagal menghapus", r);
                setRaw("bot-replay-selection", 0.f);
                m_dirty = true;
            });
        }
        else if (id == "bot-replay-management") {
            auto n = bot::selected();
            if (n.empty()) { notify("Pilih replay dulu", false); return; }
            openKeyboard("Ganti nama", n, false, nullptr, [this, n](std::string const& s, bool ok) {
                if (!ok) return;
                bool r = bot::rename(n, s);
                notify(r ? "Nama diganti" : "Gagal mengganti nama", r);
                m_dirty = true;
            });
        }
        else if (id == "labels-add-new-label") {
            openKeyboard("Teks label", "", false, nullptr, [](std::string const& s, bool ok) {
                if (!ok || s.empty()) return;
                auto l = lab::customLabels();
                l.push_back(s);
                Mod::get()->setSavedValue<std::string>("custom-labels", joinList(l, '|'));
            });
        }
        else if (id == "labels-clear-custom-labels") {
            Mod::get()->setSavedValue<std::string>("custom-labels", "");
            notify("Label kustom dihapus");
        }
    }

    // ---------------------------------------------------------------- touch
    void restorePress() {
        if (m_hasActive && m_active.press && m_active.node) m_active.node->setScale(1.f);
    }
    void flushDirty() {
        if (m_dirty || m_uiDirty) {
            m_dirty = false;
            m_uiDirty = false;
            build();
        }
    }

public:
    bool ccTouchBegan(CCTouch* t, CCEvent*) override {
        if (m_closing || !m_win) return true;
        auto w = t->getLocation();
        m_start = m_last = w;
        m_hasActive = false;
        m_axis = 0;
        m_scrollPanel = -1;
        m_mode = Mode::None;

        if (!m_ov.empty()) {
            for (int i = (int)m_ov.size() - 1; i >= 0; --i) {
                auto& ov = m_ov[i];
                if (auto h = pick(ov.hits, w)) {
                    m_active = *h; m_hasActive = true; m_mode = Mode::Press;
                    return true;
                }
                if (inside(ov.node, ov.size, w)) return true;
            }
            for (int i = (int)m_ov.size() - 1; i >= 0; --i) {
                auto cb = m_ov[i].outside;
                if (cb) { cb(); break; }
            }
            flushDirty();
            return true;
        }
        if (!inside(m_win, m_size, w)) return true;
        auto p = m_win->convertToNodeSpace(w);
        if (auto h = pick(m_hits, w)) {
            m_active = *h; m_hasActive = true;
            if (h->drag) {
                m_mode = Mode::Slider;
                h->drag(h->node->convertToNodeSpace(w));
            } else {
                m_mode = Mode::Press;
                if (h->press) h->node->setScale(0.96f);
            }
        } else if (p.y > m_size.height - HDR) {
            m_mode = Mode::Window;
        } else {
            m_mode = Mode::Scroll;
            m_scrollPanel = panelAt(w);
        }
        return true;
    }

    void ccTouchMoved(CCTouch* t, CCEvent*) override {
        if (m_closing || !m_win) return;
        auto w = t->getLocation();
        auto d = w - m_last;
        m_last = w;
        float sc = std::max(0.01f, m_scale);
        switch (m_mode) {
            case Mode::Window: {
                auto win = CCDirector::get()->getWinSize();
                float hw = m_size.width * m_scale / 2, hh = m_size.height * m_scale / 2;
                auto np = m_win->getPosition() + d;
                np.x = std::clamp(np.x, hw, std::max(hw, win.width - hw));
                np.y = std::clamp(np.y, hh, std::max(hh, win.height - hh));
                m_win->setPosition(np);
                break;
            }
            case Mode::Slider:
                if (m_active.drag) m_active.drag(m_active.node->convertToNodeSpace(w));
                break;
            case Mode::Press:
                if (ccpDistance(w, m_start) > 10.f) {
                    restorePress();
                    m_hasActive = false;
                    m_mode = Mode::Scroll;
                    m_scrollPanel = m_ov.empty() ? panelAt(m_start) : -1;
                }
                [[fallthrough]];
            case Mode::Scroll: {
                if (!m_ov.empty()) break;
                if (m_axis == 0 && (std::abs(w.x - m_start.x) > 4.f || std::abs(w.y - m_start.y) > 4.f))
                    m_axis = std::abs(w.x - m_start.x) > std::abs(w.y - m_start.y) ? 2 : 1;
                if (m_axis == 1) scrollPanel(m_scrollPanel, d.y / sc);
                else if (m_axis == 2) scrollRow(-d.x / sc);
                break;
            }
            default: break;
        }
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        if (m_closing || !m_win) return;
        restorePress();
        if (m_mode == Mode::Press && m_hasActive && m_active.click) {
            auto cb = m_active.click;
            m_hasActive = false;
            cb();
        } else if (m_mode == Mode::Slider && m_hasActive && m_active.end) {
            auto cb = m_active.end;
            m_hasActive = false;
            cb();
        } else if (m_mode == Mode::Window) {
            auto win = CCDirector::get()->getWinSize();
            Mod::get()->setSavedValue<double>("menu-fx", m_win->getPositionX() / win.width);
            Mod::get()->setSavedValue<double>("menu-fy", m_win->getPositionY() / win.height);
        }
        m_mode = Mode::None;
        m_hasActive = false;
        if (m_mode == Mode::None) flushDirty();
    }
    void ccTouchCancelled(CCTouch* t, CCEvent* e) override { ccTouchEnded(t, e); }
};

inline void toggleMenu() { TGMenu::toggle(); }
inline void openPalette() { TGMenu::openPalette(); }

} // namespace tg
