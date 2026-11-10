#pragma once

// ============================================================================
//  rift.hpp - pengganti lokal (header-only) untuk library RIFT
//  (Runtime Interpreted FormaTter) milik EclipseMenu.
//
//  Repo asli (EclipseMenu/rift) sudah tidak bisa diunduh, sehingga build lewat
//  CPM gagal. File ini mengimplementasikan subset yang dipakai mod ini:
//    - rift::Value (null / integer / float / string / boolean)
//    - rift::compile(text) -> hasil kompilasi, rift::Script::run(variabel)
//    - rift::format(teks, variabel)
//    - rift::config::addRuntimeFunction(nama, fungsi)
//  Sintaks mengikuti LABELS.md: teks biasa + ekspresi di dalam { ... }.
// ============================================================================

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace rift {

    class Value {
    public:
        Value() = default;

        static Value null() { return Value(); }
        static Value string(std::string s) { Value v; v.m_value = std::move(s); return v; }
        static Value integer(int64_t i) { Value v; v.m_value = i; return v; }
        static Value floating(double d) { Value v; v.m_value = d; return v; }
        static Value boolean(bool b) { Value v; v.m_value = b; return v; }

        template <typename T>
        static Value from(T value) {
            using D = std::decay_t<T>;
            if constexpr (std::is_same_v<D, bool>) {
                return boolean(value);
            } else if constexpr (std::is_same_v<D, std::monostate>) {
                return null();
            } else if constexpr (std::is_integral_v<D>) {
                return integer(static_cast<int64_t>(value));
            } else if constexpr (std::is_same_v<D, float>) {
                // hindari noise presisi float -> double (0.1f menjadi 0.10000000149)
                char buf[48];
                std::snprintf(buf, sizeof(buf), "%.7g", static_cast<double>(value));
                return floating(std::strtod(buf, nullptr));
            } else if constexpr (std::is_floating_point_v<D>) {
                return floating(static_cast<double>(value));
            } else if constexpr (std::is_convertible_v<D, std::string_view>) {
                return string(std::string(std::string_view(value)));
            } else {
                static_assert(!sizeof(D*), "Tipe tidak didukung oleh rift::Value::from");
            }
        }

        [[nodiscard]] bool isNull() const { return std::holds_alternative<std::monostate>(m_value); }
        [[nodiscard]] bool isInteger() const { return std::holds_alternative<int64_t>(m_value); }
        [[nodiscard]] bool isFloat() const { return std::holds_alternative<double>(m_value); }
        [[nodiscard]] bool isBoolean() const { return std::holds_alternative<bool>(m_value); }
        [[nodiscard]] bool isString() const { return std::holds_alternative<std::string>(m_value); }
        [[nodiscard]] bool isNumber() const { return isInteger() || isFloat(); }

        [[nodiscard]] int64_t getInteger() const { return isInteger() ? std::get<int64_t>(m_value) : 0; }
        [[nodiscard]] double getFloat() const { return isFloat() ? std::get<double>(m_value) : 0.0; }
        [[nodiscard]] bool getBoolean() const { return isBoolean() && std::get<bool>(m_value); }
        [[nodiscard]] std::string getString() const { return isString() ? std::get<std::string>(m_value) : std::string(); }

        [[nodiscard]] static std::string formatFloat(double d) {
            if (std::isnan(d)) return "nan";
            if (std::isinf(d)) return d < 0 ? "-inf" : "inf";
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.15g", d);
            return buf;
        }

        [[nodiscard]] std::string toString() const {
            if (isString()) return std::get<std::string>(m_value);
            if (isInteger()) return std::to_string(std::get<int64_t>(m_value));
            if (isFloat()) return formatFloat(std::get<double>(m_value));
            if (isBoolean()) return std::get<bool>(m_value) ? "true" : "false";
            return {};
        }

        [[nodiscard]] double toFloat() const {
            if (isFloat()) return std::get<double>(m_value);
            if (isInteger()) return static_cast<double>(std::get<int64_t>(m_value));
            if (isBoolean()) return std::get<bool>(m_value) ? 1.0 : 0.0;
            if (isString()) return std::strtod(std::get<std::string>(m_value).c_str(), nullptr);
            return 0.0;
        }

        [[nodiscard]] int64_t toInteger() const {
            if (isInteger()) return std::get<int64_t>(m_value);
            if (isFloat()) return static_cast<int64_t>(std::get<double>(m_value));
            if (isBoolean()) return std::get<bool>(m_value) ? 1 : 0;
            if (isString()) return static_cast<int64_t>(std::strtoll(std::get<std::string>(m_value).c_str(), nullptr, 10));
            return 0;
        }

        [[nodiscard]] bool toBoolean() const {
            if (isBoolean()) return std::get<bool>(m_value);
            if (isInteger()) return std::get<int64_t>(m_value) != 0;
            if (isFloat()) return std::get<double>(m_value) != 0.0;
            if (isString()) return !std::get<std::string>(m_value).empty();
            return false;
        }

    private:
        std::variant<std::monostate, int64_t, double, bool, std::string> m_value;
    };

    using VarMap = std::unordered_map<std::string, Value>;
    using RuntimeFunction = std::function<Value(std::span<Value>)>;

    namespace config {
        inline std::unordered_map<std::string, RuntimeFunction>& runtimeFunctions() {
            static std::unordered_map<std::string, RuntimeFunction> functions;
            return functions;
        }

        /// Daftarkan fungsi tambahan yang bisa dipanggil dari ekspresi, contoh: { cfg("key") }
        inline void addRuntimeFunction(std::string_view name, RuntimeFunction fn) {
            runtimeFunctions()[std::string(name)] = std::move(fn);
        }
    }

    namespace detail {

        struct Node;
        using NodePtr = std::unique_ptr<Node>;

        enum class Tok { End, Number, String, Ident, Op };

        struct Token {
            Tok type = Tok::End;
            std::string text;
            Value number;
        };

        struct Node {
            enum class Kind { Literal, Variable, Unary, Binary, Ternary, Call } kind = Kind::Literal;
            Value literal;
            std::string name; // nama variabel / fungsi / operator
            std::vector<NodePtr> kids;
        };

        class Lexer {
        public:
            explicit Lexer(std::string_view src) : m_src(src) {}

            bool next(Token& out, std::string& err) {
                while (m_pos < m_src.size() && std::isspace(static_cast<unsigned char>(m_src[m_pos]))) ++m_pos;
                out = Token{};
                if (m_pos >= m_src.size()) { out.type = Tok::End; return true; }

                char c = m_src[m_pos];
                if (std::isdigit(static_cast<unsigned char>(c)) ||
                    (c == '.' && m_pos + 1 < m_src.size() && std::isdigit(static_cast<unsigned char>(m_src[m_pos + 1])))) {
                    size_t start = m_pos;
                    bool isFloat = false;
                    while (m_pos < m_src.size() && (std::isdigit(static_cast<unsigned char>(m_src[m_pos])) || m_src[m_pos] == '.')) {
                        if (m_src[m_pos] == '.') isFloat = true;
                        ++m_pos;
                    }
                    std::string num(m_src.substr(start, m_pos - start));
                    out.type = Tok::Number;
                    out.number = isFloat ? Value::floating(std::strtod(num.c_str(), nullptr))
                                         : Value::integer(std::strtoll(num.c_str(), nullptr, 10));
                    return true;
                }

                if (c == '"' || c == '\'') {
                    char quote = c;
                    ++m_pos;
                    std::string str;
                    while (m_pos < m_src.size() && m_src[m_pos] != quote) {
                        char ch = m_src[m_pos++];
                        if (ch == '\\' && m_pos < m_src.size()) {
                            char esc = m_src[m_pos++];
                            switch (esc) {
                                case 'n': str += '\n'; break;
                                case 't': str += '\t'; break;
                                default: str += esc; break;
                            }
                        } else {
                            str += ch;
                        }
                    }
                    if (m_pos >= m_src.size()) { err = "string tidak ditutup"; return false; }
                    ++m_pos; // tutup kutip
                    out.type = Tok::String;
                    out.text = std::move(str);
                    return true;
                }

                if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                    size_t start = m_pos;
                    while (m_pos < m_src.size() &&
                           (std::isalnum(static_cast<unsigned char>(m_src[m_pos])) || m_src[m_pos] == '_' || m_src[m_pos] == '.')) ++m_pos;
                    out.type = Tok::Ident;
                    out.text = std::string(m_src.substr(start, m_pos - start));
                    return true;
                }

                // operator dua karakter
                if (m_pos + 1 < m_src.size()) {
                    std::string_view two = m_src.substr(m_pos, 2);
                    if (two == "==" || two == "!=" || two == "<=" || two == ">=" || two == "&&" || two == "||") {
                        out.type = Tok::Op;
                        out.text = std::string(two);
                        m_pos += 2;
                        return true;
                    }
                }
                if (std::string_view("+-*/%()<>,?:!").find(c) != std::string_view::npos) {
                    out.type = Tok::Op;
                    out.text = std::string(1, c);
                    ++m_pos;
                    return true;
                }

                err = std::string("karakter tidak dikenal '") + c + "'";
                return false;
            }

        private:
            std::string_view m_src;
            size_t m_pos = 0;
        };

        class Parser {
        public:
            explicit Parser(std::string_view src) : m_lexer(src) {}

            NodePtr parse(std::string& err) {
                if (!advance(err)) return nullptr;
                auto node = ternary(err);
                if (!node) return nullptr;
                if (m_tok.type != Tok::End) { err = "token tidak terduga '" + m_tok.text + "'"; return nullptr; }
                return node;
            }

        private:
            Lexer m_lexer;
            Token m_tok;

            bool advance(std::string& err) { return m_lexer.next(m_tok, err); }
            bool isOp(std::string_view op) const { return m_tok.type == Tok::Op && m_tok.text == op; }

            static NodePtr make(Node::Kind kind, std::string name = {}) {
                auto n = std::make_unique<Node>();
                n->kind = kind;
                n->name = std::move(name);
                return n;
            }

            NodePtr ternary(std::string& err) {
                auto cond = logicalOr(err);
                if (!cond) return nullptr;
                if (!isOp("?")) return cond;
                if (!advance(err)) return nullptr;
                auto a = ternary(err);
                if (!a) return nullptr;
                if (!isOp(":")) { err = "diharapkan ':' pada operator ternary"; return nullptr; }
                if (!advance(err)) return nullptr;
                auto b = ternary(err);
                if (!b) return nullptr;
                auto node = make(Node::Kind::Ternary);
                node->kids.push_back(std::move(cond));
                node->kids.push_back(std::move(a));
                node->kids.push_back(std::move(b));
                return node;
            }

            template <typename Next>
            NodePtr binaryLevel(std::string& err, std::initializer_list<std::string_view> ops, Next next) {
                auto left = (this->*next)(err);
                if (!left) return nullptr;
                while (true) {
                    std::string_view matched;
                    for (auto op : ops) if (isOp(op)) { matched = op; break; }
                    if (matched.empty()) return left;
                    std::string opName(matched);
                    if (!advance(err)) return nullptr;
                    auto right = (this->*next)(err);
                    if (!right) return nullptr;
                    auto node = make(Node::Kind::Binary, opName);
                    node->kids.push_back(std::move(left));
                    node->kids.push_back(std::move(right));
                    left = std::move(node);
                }
            }

            NodePtr logicalOr(std::string& err) { return binaryLevel(err, {"||"}, &Parser::logicalAnd); }
            NodePtr logicalAnd(std::string& err) { return binaryLevel(err, {"&&"}, &Parser::equality); }
            NodePtr equality(std::string& err) { return binaryLevel(err, {"==", "!="}, &Parser::comparison); }
            NodePtr comparison(std::string& err) { return binaryLevel(err, {"<=", ">=", "<", ">"}, &Parser::additive); }
            NodePtr additive(std::string& err) { return binaryLevel(err, {"+", "-"}, &Parser::multiplicative); }
            NodePtr multiplicative(std::string& err) { return binaryLevel(err, {"*", "/", "%"}, &Parser::unary); }

            NodePtr unary(std::string& err) {
                if (isOp("-") || isOp("!")) {
                    std::string op = m_tok.text;
                    if (!advance(err)) return nullptr;
                    auto operand = unary(err);
                    if (!operand) return nullptr;
                    auto node = make(Node::Kind::Unary, op);
                    node->kids.push_back(std::move(operand));
                    return node;
                }
                return primary(err);
            }

            NodePtr primary(std::string& err) {
                if (m_tok.type == Tok::Number) {
                    auto node = make(Node::Kind::Literal);
                    node->literal = m_tok.number;
                    if (!advance(err)) return nullptr;
                    return node;
                }
                if (m_tok.type == Tok::String) {
                    auto node = make(Node::Kind::Literal);
                    node->literal = Value::string(m_tok.text);
                    if (!advance(err)) return nullptr;
                    return node;
                }
                if (m_tok.type == Tok::Ident) {
                    std::string name = m_tok.text;
                    if (!advance(err)) return nullptr;
                    if (name == "true" || name == "false") {
                        auto node = make(Node::Kind::Literal);
                        node->literal = Value::boolean(name == "true");
                        return node;
                    }
                    if (name == "null") return make(Node::Kind::Literal);
                    if (isOp("(")) {
                        if (!advance(err)) return nullptr;
                        auto call = make(Node::Kind::Call, name);
                        if (!isOp(")")) {
                            while (true) {
                                auto arg = ternary(err);
                                if (!arg) return nullptr;
                                call->kids.push_back(std::move(arg));
                                if (isOp(",")) { if (!advance(err)) return nullptr; continue; }
                                break;
                            }
                        }
                        if (!isOp(")")) { err = "diharapkan ')' setelah argumen fungsi " + name; return nullptr; }
                        if (!advance(err)) return nullptr;
                        return call;
                    }
                    return make(Node::Kind::Variable, name);
                }
                if (isOp("(")) {
                    if (!advance(err)) return nullptr;
                    auto inner = ternary(err);
                    if (!inner) return nullptr;
                    if (!isOp(")")) { err = "diharapkan ')'"; return nullptr; }
                    if (!advance(err)) return nullptr;
                    return inner;
                }
                err = m_tok.type == Tok::End ? "ekspresi tidak lengkap" : "token tidak terduga '" + m_tok.text + "'";
                return nullptr;
            }
        };

        inline std::string padString(const std::string& str, int64_t len, const std::string& padChar, int mode) {
            if (len <= static_cast<int64_t>(str.size()) || padChar.empty()) return str;
            size_t total = static_cast<size_t>(len) - str.size();
            char c = padChar[0];
            if (mode == 0) return std::string(total, c) + str;
            if (mode == 1) return str + std::string(total, c);
            size_t left = total / 2;
            return std::string(left, c) + str + std::string(total - left, c);
        }

        inline Value callBuiltin(const std::string& name, std::span<Value> a, bool& found) {
            found = true;
            auto arg = [&](size_t i) -> Value { return i < a.size() ? a[i] : Value::null(); };

            // --- string
            if (name == "len") return Value::integer(static_cast<int64_t>(arg(0).toString().size()));
            if (name == "substr") {
                auto s = arg(0).toString();
                int64_t start = std::clamp<int64_t>(arg(1).toInteger(), 0, static_cast<int64_t>(s.size()));
                int64_t end = a.size() > 2 ? std::clamp<int64_t>(arg(2).toInteger(), start, static_cast<int64_t>(s.size()))
                                           : static_cast<int64_t>(s.size());
                return Value::string(s.substr(static_cast<size_t>(start), static_cast<size_t>(end - start)));
            }
            if (name == "toUpper") {
                auto s = arg(0).toString();
                std::ranges::transform(s, s.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
                return Value::string(std::move(s));
            }
            if (name == "toLower") {
                auto s = arg(0).toString();
                std::ranges::transform(s, s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                return Value::string(std::move(s));
            }
            if (name == "trim") {
                auto s = arg(0).toString();
                size_t b = 0, e = s.size();
                while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
                while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
                return Value::string(s.substr(b, e - b));
            }
            if (name == "replace") {
                auto s = arg(0).toString();
                auto from = arg(1).toString();
                auto to = arg(2).toString();
                if (from.empty()) return Value::string(std::move(s));
                std::string out;
                size_t pos = 0;
                while (true) {
                    size_t f = s.find(from, pos);
                    if (f == std::string::npos) { out += s.substr(pos); break; }
                    out += s.substr(pos, f - pos);
                    out += to;
                    pos = f + from.size();
                }
                return Value::string(std::move(out));
            }
            if (name == "leftPad" || name == "rightPad" || name == "middlePad") {
                auto fill = a.size() > 2 ? arg(2).toString() : std::string(" ");
                int mode = name == "leftPad" ? 0 : (name == "rightPad" ? 1 : 2);
                return Value::string(padString(arg(0).toString(), arg(1).toInteger(), fill, mode));
            }

            // --- acak & pembulatan
            if (name == "random") {
                static std::mt19937_64 rng{std::random_device{}()};
                double lo = arg(0).toFloat(), hi = arg(1).toFloat();
                if (lo > hi) std::swap(lo, hi);
                return Value::floating(std::uniform_real_distribution<double>(lo, hi)(rng));
            }
            if (name == "round") return Value::integer(static_cast<int64_t>(std::llround(arg(0).toFloat())));
            if (name == "floor") return Value::integer(static_cast<int64_t>(std::floor(arg(0).toFloat())));
            if (name == "ceil") return Value::integer(static_cast<int64_t>(std::ceil(arg(0).toFloat())));

            // --- matematika
            if (name == "abs") {
                auto v = arg(0);
                return v.isInteger() ? Value::integer(std::llabs(v.getInteger())) : Value::floating(std::fabs(v.toFloat()));
            }
            if (name == "min" || name == "max" || name == "sum" || name == "avg") {
                if (a.empty()) return Value::integer(0);
                bool allInt = std::ranges::all_of(a, [](const Value& v) { return v.isInteger(); });
                double acc = a[0].toFloat();
                for (size_t i = 1; i < a.size(); ++i) {
                    double x = a[i].toFloat();
                    if (name == "min") acc = std::min(acc, x);
                    else if (name == "max") acc = std::max(acc, x);
                    else acc += x;
                }
                if (name == "avg") return Value::floating(acc / static_cast<double>(a.size()));
                return allInt ? Value::integer(static_cast<int64_t>(acc)) : Value::floating(acc);
            }
            if (name == "sqrt") return Value::floating(std::sqrt(arg(0).toFloat()));
            if (name == "pow") return Value::floating(std::pow(arg(0).toFloat(), arg(1).toFloat()));
            if (name == "sin") return Value::floating(std::sin(arg(0).toFloat()));
            if (name == "cos") return Value::floating(std::cos(arg(0).toFloat()));
            if (name == "tan") return Value::floating(std::tan(arg(0).toFloat()));
            if (name == "precision") {
                int digits = static_cast<int>(std::clamp<int64_t>(arg(1).toInteger(), 0, 15));
                char buf[96];
                std::snprintf(buf, sizeof(buf), "%.*f", digits, arg(0).toFloat());
                return Value::string(buf);
            }

            found = false;
            return Value::null();
        }

        inline Value evaluate(const Node& node, const VarMap& vars, std::string& err);

        inline Value applyBinary(const std::string& op, const Value& l, const Value& r) {
            if (op == "+") {
                if (l.isString() || r.isString()) return Value::string(l.toString() + r.toString());
                if (l.isFloat() || r.isFloat()) return Value::floating(l.toFloat() + r.toFloat());
                return Value::integer(l.toInteger() + r.toInteger());
            }
            if (op == "*") {
                if (l.isString() && !r.isString()) {
                    std::string out;
                    for (int64_t i = 0; i < std::min<int64_t>(r.toInteger(), 10000); ++i) out += l.getString();
                    return Value::string(std::move(out));
                }
                if (r.isString() && !l.isString()) return applyBinary("*", r, l);
                if (l.isFloat() || r.isFloat()) return Value::floating(l.toFloat() * r.toFloat());
                return Value::integer(l.toInteger() * r.toInteger());
            }
            if (op == "-") {
                if (l.isFloat() || r.isFloat()) return Value::floating(l.toFloat() - r.toFloat());
                return Value::integer(l.toInteger() - r.toInteger());
            }
            if (op == "/") {
                if (r.toFloat() == 0.0) return Value::integer(0);
                if (!l.isFloat() && !r.isFloat() && l.toInteger() % r.toInteger() == 0)
                    return Value::integer(l.toInteger() / r.toInteger());
                return Value::floating(l.toFloat() / r.toFloat());
            }
            if (op == "%") {
                if (!l.isFloat() && !r.isFloat()) {
                    if (r.toInteger() == 0) return Value::integer(0);
                    return Value::integer(l.toInteger() % r.toInteger());
                }
                if (r.toFloat() == 0.0) return Value::integer(0);
                return Value::floating(std::fmod(l.toFloat(), r.toFloat()));
            }

            // perbandingan
            int cmp = 0;
            if (l.isString() && r.isString()) {
                cmp = l.getString().compare(r.getString());
            } else if (l.isString() || r.isString()) {
                cmp = l.toString().compare(r.toString());
            } else {
                double a = l.toFloat(), b = r.toFloat();
                cmp = a < b ? -1 : (a > b ? 1 : 0);
            }
            if (op == "==") return Value::boolean(cmp == 0);
            if (op == "!=") return Value::boolean(cmp != 0);
            if (op == "<") return Value::boolean(cmp < 0);
            if (op == ">") return Value::boolean(cmp > 0);
            if (op == "<=") return Value::boolean(cmp <= 0);
            if (op == ">=") return Value::boolean(cmp >= 0);
            return Value::null();
        }

        inline Value evaluate(const Node& node, const VarMap& vars, std::string& err) {
            switch (node.kind) {
                case Node::Kind::Literal: return node.literal;
                case Node::Kind::Variable: {
                    auto it = vars.find(node.name);
                    return it == vars.end() ? Value::null() : it->second;
                }
                case Node::Kind::Unary: {
                    auto v = evaluate(*node.kids[0], vars, err);
                    if (node.name == "!") return Value::boolean(!v.toBoolean());
                    if (v.isFloat()) return Value::floating(-v.getFloat());
                    return Value::integer(-v.toInteger());
                }
                case Node::Kind::Ternary:
                    return evaluate(*node.kids[0], vars, err).toBoolean()
                        ? evaluate(*node.kids[1], vars, err)
                        : evaluate(*node.kids[2], vars, err);
                case Node::Kind::Binary: {
                    if (node.name == "&&") {
                        if (!evaluate(*node.kids[0], vars, err).toBoolean()) return Value::boolean(false);
                        return Value::boolean(evaluate(*node.kids[1], vars, err).toBoolean());
                    }
                    if (node.name == "||") {
                        if (evaluate(*node.kids[0], vars, err).toBoolean()) return Value::boolean(true);
                        return Value::boolean(evaluate(*node.kids[1], vars, err).toBoolean());
                    }
                    auto l = evaluate(*node.kids[0], vars, err);
                    auto r = evaluate(*node.kids[1], vars, err);
                    return applyBinary(node.name, l, r);
                }
                case Node::Kind::Call: {
                    std::vector<Value> args;
                    args.reserve(node.kids.size());
                    for (auto& kid : node.kids) args.push_back(evaluate(*kid, vars, err));

                    auto& custom = config::runtimeFunctions();
                    if (auto it = custom.find(node.name); it != custom.end())
                        return it->second(std::span<Value>(args));

                    bool found = false;
                    auto result = callBuiltin(node.name, std::span<Value>(args), found);
                    if (!found) {
                        err = "fungsi tidak dikenal: " + node.name;
                        return Value::string("<" + err + ">");
                    }
                    return result;
                }
            }
            return Value::null();
        }

    } // namespace detail

    /// Skrip hasil kompilasi: potongan teks biasa + ekspresi { ... }
    class Script {
    public:
        struct Part {
            bool isExpression = false;
            std::string text;
            detail::NodePtr node;
        };

        [[nodiscard]] std::string run(const VarMap& vars) const {
            std::string out;
            for (auto& part : m_parts) {
                if (!part.isExpression) { out += part.text; continue; }
                std::string err;
                out += detail::evaluate(*part.node, vars, err).toString();
            }
            return out;
        }

        std::vector<Part> m_parts;
    };

    /// Hasil dari rift::compile. Pemanggil yang memiliki Script* dari getValue().
    class CompileResult {
    public:
        static CompileResult ok(Script* script) { CompileResult r; r.m_script = script; return r; }
        static CompileResult error(std::string message) { CompileResult r; r.m_message = std::move(message); return r; }

        explicit operator bool() const { return m_script != nullptr; }
        [[nodiscard]] Script* getValue() const { return m_script; }
        [[nodiscard]] const std::string& getMessage() const { return m_message; }

    private:
        Script* m_script = nullptr;
        std::string m_message;
    };

    inline CompileResult compile(std::string_view source) {
        auto script = std::make_unique<Script>();
        std::string literal;
        size_t i = 0;

        auto flushLiteral = [&] {
            if (literal.empty()) return;
            Script::Part part;
            part.text = std::move(literal);
            literal.clear();
            script->m_parts.push_back(std::move(part));
        };

        while (i < source.size()) {
            char c = source[i];
            if (c != '{') { literal += c; ++i; continue; }

            // cari penutup '}' dengan memperhatikan string
            size_t j = i + 1;
            char quote = 0;
            while (j < source.size()) {
                char ch = source[j];
                if (quote) {
                    if (ch == '\\') ++j;
                    else if (ch == quote) quote = 0;
                } else if (ch == '"' || ch == '\'') {
                    quote = ch;
                } else if (ch == '}') {
                    break;
                }
                ++j;
            }
            if (j >= source.size()) return CompileResult::error("tanda '{' tidak ditutup dengan '}'");

            flushLiteral();
            std::string err;
            detail::Parser parser(source.substr(i + 1, j - i - 1));
            auto node = parser.parse(err);
            if (!node) return CompileResult::error(err.empty() ? "ekspresi tidak valid" : err);

            Script::Part part;
            part.isExpression = true;
            part.node = std::move(node);
            script->m_parts.push_back(std::move(part));
            i = j + 1;
        }
        flushLiteral();
        return CompileResult::ok(script.release());
    }

    inline std::string format(std::string_view source, const VarMap& vars) {
        auto res = compile(source);
        if (!res) return "<" + res.getMessage() + ">";
        std::unique_ptr<Script> script(res.getValue());
        return script->run(vars);
    }

} // namespace rift
