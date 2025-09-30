#pragma once
#include <string>
#include <vector>
#include <variant>
#include <initializer_list>
#include <stdexcept>
#include <sstream>
#include <type_traits>
#include <cctype>
#include <algorithm>
#include <utility>
#include <cstddef>
#include <locale>
#include <codecvt>

namespace jsn {

struct parse_error : std::runtime_error { using std::runtime_error::runtime_error; };
struct type_error  : std::runtime_error { using std::runtime_error::runtime_error; };
struct out_of_range : std::runtime_error { using std::runtime_error::runtime_error; };

template<typename CharT>
class basic_json {
public:
    enum class value_t { null, boolean, number, string, array, object };
    using string_t = std::basic_string<CharT>;
    using array_t = std::vector<basic_json>;
    using object_t = std::vector<std::pair<string_t, basic_json>>;
    using storage_t = std::variant<std::nullptr_t, bool, double, string_t, array_t, object_t>;

    basic_json() noexcept : m_type(value_t::null), m_storage(nullptr) {}
    basic_json(std::nullptr_t) noexcept : m_type(value_t::null), m_storage(nullptr) {}
    basic_json(bool b) : m_type(value_t::boolean), m_storage(b) {}
    basic_json(double d) : m_type(value_t::number), m_storage(d) {}
    basic_json(const CharT* s) : m_type(value_t::string), m_storage(string_t(s)) {}
    basic_json(const string_t& s) : m_type(value_t::string), m_storage(s) {}
    basic_json(const array_t& arr) : m_type(value_t::array), m_storage(arr) {}
    basic_json(const object_t& obj) : m_type(value_t::object), m_storage(obj) {}

    template<typename I>
    basic_json(I i) requires(std::integral<I> && !std::same_as<I, bool>)
        : m_type(value_t::number), m_storage(static_cast<double>(i)) {}

    template<typename T>
    basic_json(const T& val) requires(!std::same_as<std::decay_t<T>, basic_json> && !std::is_arithmetic_v<T>)
        : m_type(value_t::null), m_storage(nullptr) { to_json(*this, val); }

    basic_json(std::initializer_list<basic_json> init) {
        bool is_obj = init.size() > 0;
        for (const auto& el : init) {
            if (!el.is_array()) {
                is_obj = false;
                break;
            }
            const auto& sub = el.template get<array_t>();
            if (sub.size() != 2 || !sub[0].is_string()) {
                is_obj = false;
                break;
            }
        }

        if (is_obj) {
            object_t obj;
            obj.reserve(init.size());
            for (const auto& el : init) {
                const auto& sub = el.template get<array_t>();
                obj.emplace_back(sub[0].template get<string_t>(), sub[1]);
            }
            m_type = value_t::object;
            m_storage = obj;
        } else {
            m_type = value_t::array;
            m_storage = array_t(init);
        }
    }

    value_t type() const noexcept { return m_type; }
    bool is_null() const noexcept { return m_type == value_t::null; }
    bool is_boolean() const noexcept { return m_type == value_t::boolean; }
    bool is_number() const noexcept { return m_type == value_t::number; }
    bool is_string() const noexcept { return m_type == value_t::string; }
    bool is_array() const noexcept { return m_type == value_t::array; }
    bool is_object() const noexcept { return m_type == value_t::object; }

    size_t size() const noexcept {
        switch (m_type) {
            case value_t::array:  return std::get<array_t>(m_storage).size();
            case value_t::object: return std::get<object_t>(m_storage).size();
            default: return 0;
        }
    }
    bool empty() const noexcept { return size() == 0; }

    template<typename T>
    T get() const {
        if constexpr (std::is_same_v<T, bool>) {
            if (!is_boolean()) throw type_error("not boolean");
            return std::get<bool>(m_storage);
        } else if constexpr (std::is_arithmetic_v<T>) {
            if (!is_number()) throw type_error("not number");
            return static_cast<T>(std::get<double>(m_storage));
        } else if constexpr (std::is_same_v<T, string_t>) {
            if (!is_string()) throw type_error("not string");
            return std::get<string_t>(m_storage);
        } else if constexpr (std::is_same_v<T, array_t>) {
            if (!is_array()) throw type_error("not array");
            return std::get<array_t>(m_storage);
        } else if constexpr (std::is_same_v<T, object_t>) {
            if (!is_object()) throw type_error("not object");
            return std::get<object_t>(m_storage);
        } else if constexpr (std::is_class_v<T>) {
            T tmp;
            from_json(*this, tmp);
            return tmp;
        } else {
            static_assert(!sizeof(T), "unsupported type");
        }
    }

    template<typename T> operator T() const { return get<T>(); }
    template<typename T> void get_to(T& val) const { val = get<T>(); }

    bool get_bool() const { return get<bool>(); }
    double get_number() const { return get<double>(); }
    const string_t get_string() const { return get<string_t>(); }
    const array_t get_array() const { return get<array_t>(); }
    const object_t get_object() const { return get<object_t>(); }

    basic_json& operator[](size_t i) { return at(i); }
    const basic_json& at(size_t i) const {
        if (!is_array()) throw type_error("not array");
        const auto& a = std::get<array_t>(m_storage);
        if (i >= a.size()) throw out_of_range("index out of range");
        return a[i];
    }
    basic_json& at(size_t i) {
        return const_cast<basic_json&>(static_cast<const basic_json*>(this)->at(i));
    }

    basic_json& operator[](const CharT* key) { return operator[](string_t(key)); }
    const basic_json& operator[](const CharT* key) const { return at(string_t(key)); }
    
    basic_json& operator[](const string_t& key) {
        if (!is_object()) { m_type = value_t::object; m_storage = object_t(); }
        auto& obj = std::get<object_t>(m_storage);
        for (auto& kv : obj) if (kv.first == key) return kv.second;
        obj.emplace_back(key, basic_json());
        return obj.back().second;
    }
    const basic_json& at(const string_t& key) const {
        if (!is_object()) throw type_error("not object");
        const auto& obj = std::get<object_t>(m_storage);
        for (const auto& kv : obj) if (kv.first == key) return kv.second;
        throw out_of_range("key not found");
    }

    bool contains(const string_t& key) const {
        if (!is_object()) return false;
        const auto& obj = std::get<object_t>(m_storage);
        for (const auto& kv : obj) if (kv.first == key) return true;
        return false;
    }

    auto find(const basic_json& value) {
        if (is_array()) {
            auto& arr = std::get<array_t>(m_storage);
            auto it = std::find(arr.begin(), arr.end(), value);
            return iterator(it);
        }
        if (!is_object()) throw type_error("not container");
        auto& obj = std::get<object_t>(m_storage);
        auto oit = std::find_if(obj.begin(), obj.end(),
                                [&](auto const& kv){ return kv.first == value.get_string(); });
        return iterator(oit);
    }

    auto find(const basic_json& value) const {
        if (is_array()) {
            const auto& arr = std::get<array_t>(m_storage);
            auto it = std::find(arr.cbegin(), arr.cend(), value);
            return const_iterator(it);
        }
        if (!is_object()) throw type_error("not container");
        const auto& obj = std::get<object_t>(m_storage);
        auto oit = std::find_if(obj.cbegin(), obj.cend(), [&](auto const& kv){ return kv.first == value.get_string(); });
        return const_iterator(oit);
    }

    void clear() noexcept {
        if (is_array())  std::get<array_t>(m_storage).clear();
        if (is_object()) std::get<object_t>(m_storage).clear();
    }

    void push_back(const basic_json& j) {
        if (!is_array()) throw type_error("not array");
        std::get<array_t>(m_storage).push_back(j);
    }

    void erase(size_t i) {
        if (!is_array()) throw type_error("not array");
        auto& a = std::get<array_t>(m_storage);
        a.erase(a.begin() + i);
    }

    void erase(const string_t& key) {
        if (!is_object()) throw type_error("not object");
        auto& obj = std::get<object_t>(m_storage);
        auto it = std::find_if(obj.begin(), obj.end(), [&](auto const& kv){ return kv.first == key; });
        if (it != obj.end()) {
            obj.erase(it);
        }
    }

    struct proxy {
        string_t key;
        basic_json& value;

        proxy(const string_t& k, basic_json& v) : key(k), value(v) {}
        explicit proxy(basic_json& v) : key(), value(v) {}

        proxy at(const string_t& k) { return proxy(k, value.at(k)); }
        proxy at(const CharT* k) {  return at(string_t(k)); }

        bool is_null() const noexcept { return value.is_null(); }
        bool is_boolean() const noexcept { return value.is_boolean(); }
        bool is_number() const noexcept { return value.is_number(); }
        bool is_string() const noexcept { return value.is_string(); }
        bool is_array() const noexcept { return value.is_array(); }
        bool is_object() const noexcept { return value.is_object(); }

        string_t dump(int indent = -1) const { return value.dump(indent); }

        proxy& operator=(std::nullptr_t) { value = nullptr; return *this; }
        proxy& operator=(bool b) { value = b; return *this; }
        proxy& operator=(int i) { value = i; return *this; }
        proxy& operator=(double d) { value = d; return *this; }
        proxy& operator=(const CharT* s) { value = string_t(s); return *this; }
        proxy& operator=(const string_t& s) { value = s; return *this; }
        proxy& operator=(const basic_json& j) { value = j; return *this; }
        proxy operator[](const CharT* key) { return proxy(string_t(key), this->operator[](string_t(key)));}

        template<typename T> operator T() const { return value.template get<T>(); }
        template<typename T> T get() const { return value.template get<T>(); }

        basic_json* operator->() { return &value; }
        const basic_json* operator->() const { return &value; }
    };

    struct const_proxy {
        string_t key;
        const basic_json& value;

        const_proxy(const string_t& k, const basic_json& v) : key(k), value(v) {}
        explicit const_proxy(const basic_json& v) : key(), value(v) {}

        const_proxy at(const string_t& k) const { return const_proxy(k, value.at(k)); }
        const_proxy at(const CharT* k) const { return at(string_t(k)); }

        bool is_null() const noexcept { return value.is_null(); }
        bool is_boolean() const noexcept { return value.is_boolean(); }
        bool is_number() const noexcept { return value.is_number(); }
        bool is_string() const noexcept { return value.is_string(); }
        bool is_array() const noexcept { return value.is_array(); }
        bool is_object() const noexcept { return value.is_object(); }

        string_t dump(int indent = -1) const { return value.dump(indent); }

        const_proxy operator[](const CharT* key) const { return const_proxy(string_t(key), this->at(string_t(key))); }

        template<typename T> operator T() const { return value.template get<T>(); }
        template<typename T> T get() const { return value.template get<T>(); }

        const basic_json* operator->() const { return &value; }
    };

    struct iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = basic_json;
        using pointer = proxy;
        using reference = proxy;

        std::variant<typename array_t::iterator, typename object_t::iterator> it;

        iterator(typename array_t::iterator a) : it(a) {}
        iterator(typename object_t::iterator o) : it(o) {}

        iterator& operator++() { 
            std::visit([](auto& i) { i++; }, it); 
            return *this;
        }

        bool operator==(const iterator& other) const { return it == other.it; }
        bool operator!=(const iterator& other) const { return !(*this == other); }

        proxy operator*() { 
            return std::visit([](auto& i) -> proxy {
                using T = std::decay_t<decltype(i)>;
                if constexpr (std::is_same_v<T, typename array_t::iterator>) {
                    return proxy{ string_t(), *i };
                } else {
                    return proxy{ i->first, i->second };
                }
            }, it);
        }
    };

    struct const_iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = const basic_json;
        using pointer = const_proxy;
        using reference = const_proxy;

        std::variant<typename array_t::const_iterator, typename object_t::const_iterator> it;

        const_iterator(typename array_t::const_iterator a) : it(a) {}
        const_iterator(typename object_t::const_iterator o) : it(o) {}

        const_iterator& operator++() { 
            std::visit([](auto& i) { i++; }, it); 
            return *this;
        }

        bool operator==(const const_iterator& other) const { return it == other.it; }
        bool operator!=(const const_iterator& other) const { return !(*this == other); }

        const_proxy operator*() const { 
            return std::visit([](auto& i) -> const_proxy {
                using T = std::decay_t<decltype(i)>;
                if constexpr (std::is_same_v<T, typename array_t::const_iterator>) {
                    return const_proxy{ string_t(), *i };
                } else {
                    return const_proxy{ i->first, i->second };
                }
            }, it);
        }
    };

    iterator begin() { 
        if (is_array()) return iterator{ std::get<array_t>(m_storage).begin() };
        if (is_object()) return iterator{ std::get<object_t>(m_storage).begin() };
        throw type_error("not container");
    }

    iterator end() { 
        if (is_array()) return iterator{ std::get<array_t>(m_storage).end() };
        if (is_object()) return iterator{ std::get<object_t>(m_storage).end() };
        throw type_error("not container");
    }

    const_iterator begin() const { 
        if (is_array()) return const_iterator{ std::get<array_t>(m_storage).cbegin() };
        if (is_object()) return const_iterator{ std::get<object_t>(m_storage).cbegin() };
        throw type_error("not container");
    }

    const_iterator end() const { 
        if (is_array()) return const_iterator{ std::get<array_t>(m_storage).cend() };
        if (is_object()) return const_iterator{ std::get<object_t>(m_storage).cend() };
        throw type_error("not container");
    }

    string_t dump(int indent = -1) const {
        std::basic_ostringstream<CharT> oss;
        dump_value(oss, *this, indent, 0);
        return oss.str();
    }

    static basic_json parse(const string_t& s) {
        size_t i = 0;
        auto j = parse_value(s, i);
        skip_ws(s, i);
        if (i != s.size()) throw parse_error("extra characters");
        return j;
    }

    friend bool operator==(const basic_json& a, const basic_json& b) noexcept {
        return a.m_type == b.m_type && a.m_storage == b.m_storage;
    }

    friend bool operator!=(const basic_json& a, const basic_json& b) noexcept { return !(a == b); }

private:
    value_t m_type;
    storage_t m_storage;

    static void skip_ws(const string_t& s, size_t& i) {
        while (i < s.size() && std::isspace(static_cast<CharT>(s[i]))) i++;
    }

    static basic_json parse_value(const string_t& s, size_t& i) {
        skip_ws(s, i);
        if (i >= s.size()) throw parse_error("unexpected end");

        const auto c = s[i];
        if (c == 'n' && s.compare(i, 4, null_literal()) == 0) {
            i += 4;
            return nullptr;
        }
        if (c == 't' && s.compare(i, 4, true_literal()) == 0) {
            i += 4;
            return true;
        }
        if (c == 'f' && s.compare(i, 5, false_literal()) == 0) {
            i += 5;
            return false;
        }
        if (c == '"') return basic_json(parse_string(s, i));
        if (c == '[') return parse_array(s, i);
        if (c == '{') return parse_object(s, i);
        if (c == '-' || (c >= '0' && c <= '9')) return parse_number(s, i);
        throw parse_error("invalid token");
    }

    static string_t parse_string(const string_t& s, size_t& i) {
        i++;
        string_t result;
        while (i < s.size()) {
            const auto c = s[i++];
            if (c == '"') return result;
            if (c == '\\' && i < s.size()) {
                const auto esc = s[i++];
                switch (esc) {
                    case '"':  result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    default:   result += esc; break;
                }
            } else {
                result += c;
            }
        }
        throw parse_error("unterminated string");
    }

    static basic_json parse_array(const string_t& s, size_t& i) {
        i++;
        array_t arr;
        skip_ws(s, i);
        if (i < s.size() && s[i] == ']') {
            i++;
            return arr;
        }
        while (true) {
            arr.push_back(parse_value(s, i));
            skip_ws(s, i);
            if (i < s.size() && s[i] == ']') {
                i++;
                break;
            }
            if (i >= s.size() || s[i] != ',') throw parse_error("expected ','");
            i++;
            skip_ws(s, i);
        }
        return arr;
    }

    static basic_json parse_object(const string_t& s, size_t& i) {
        i++;
        object_t obj;
        skip_ws(s, i);
        if (i < s.size() && s[i] == '}') {
            i++;
            return obj;
        }
        while (true) {
            auto key = parse_string(s, i);
            skip_ws(s, i);
            if (i >= s.size() || s[i] != ':') throw parse_error("expected ':'");
            i++;
            auto value = parse_value(s, i);
            obj.emplace_back(std::move(key), std::move(value));
            skip_ws(s, i);
            if (i < s.size() && s[i] == '}') {
                i++;
                break;
            }
            if (i >= s.size() || s[i] != ',') throw parse_error("expected ','");
            i++;
            skip_ws(s, i);
        }
        return obj;
    }

    static basic_json parse_number(const string_t& s, size_t& i) {
        const size_t start = i;
        if (s[i] == '-') i++;
        while (i < s.size() && std::isdigit(static_cast<CharT>(s[i]))) i++;
        if (i < s.size() && s[i] == '.') {
            i++;
            while (i < s.size() && std::isdigit(static_cast<CharT>(s[i]))) i++;
        }
        if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
            i++;
            if (i < s.size() && (s[i] == '+' || s[i] == '-')) i++;
            while (i < s.size() && std::isdigit(static_cast<CharT>(s[i]))) i++;
        }
        const auto num_str = s.substr(start, i - start);
        try {
            return std::stod(num_str);
        } catch (...) {
            throw parse_error("invalid number");
        }
    }

    static void dump_value(std::basic_ostringstream<CharT>& oss, const basic_json& j, int indent, int level) {
        oss.imbue(std::locale::classic());
        switch (j.m_type) {
            case value_t::null:
                oss << null_literal();
                break;
            case value_t::boolean:
                oss << (j.template get<bool>() ? true_literal() : false_literal());
                break;
            case value_t::number:
                dump_number(oss, j.get<double>());
                break;
            case value_t::string:
                oss << '"' << escape_string(j.template get<string_t>()) << '"';
                break;
            case value_t::array:
                dump_array(oss, j.template get<array_t>(), indent, level);
                break;
            case value_t::object:
                dump_object(oss, j.template get<object_t>(), indent, level);
                break;
        }
    }

    static void dump_number(std::basic_ostringstream<CharT>& oss, double d) {
        if (d == std::trunc(d)) oss << static_cast<int64_t>(d);
        else oss << d;
    }

    static void dump_array(std::basic_ostringstream<CharT>& oss, const array_t& arr, int indent, int level) {
        oss << '[';
        if (indent >= 0 && !arr.empty()) oss << '\n';
        for (size_t i = 0; i < arr.size(); i++) {
            if (indent >= 0) oss << string_t((level + 1) * indent, ' ');
            dump_value(oss, arr[i], indent, level + 1);
            if (i != arr.size() - 1) oss << ',';
            if (indent >= 0) oss << '\n';
        }
        if (indent >= 0 && !arr.empty()) oss << string_t(level * indent, ' ');
        oss << ']';
    }

    static void dump_object(std::basic_ostringstream<CharT>& oss, const object_t& obj, int indent, int level) {
        oss << '{';
        if (indent >= 0 && !obj.empty()) oss << '\n';
        for (size_t i = 0; i < obj.size(); i++) {
            if (indent >= 0) oss << string_t((level + 1) * indent, ' ');
            oss << '"' << escape_string(obj[i].first) << "\": ";
            dump_value(oss, obj[i].second, indent, level + 1);
            if (i != obj.size() - 1) oss << ',';
            if (indent >= 0) oss << '\n';
        }
        if (indent >= 0 && !obj.empty()) oss << string_t(level * indent, ' ');
        oss << '}';
    }

    static string_t escape_string(const string_t& s) {
        string_t result;
        for (auto c : s) {
            switch (c) {
                case '"':  result += '\\'; result += '"';  break;
                case '\\': result += '\\'; result += '\\'; break;
                case '\b': result += '\\'; result += 'b';  break;
                case '\f': result += '\\'; result += 'f';  break;
                case '\n': result += '\\'; result += 'n';  break;
                case '\r': result += '\\'; result += 'r';  break;
                case '\t': result += '\\'; result += 't';  break;
                default:   result += c;     break;
            }
        }
        return result;
    }

    static constexpr const CharT* null_literal() {
        if constexpr (std::is_same_v<CharT, char>) return "null";
        else return L"null";
    }

    static constexpr const CharT* true_literal() {
        if constexpr (std::is_same_v<CharT, char>) return "true";
        else return L"true";
    }

    static constexpr const CharT* false_literal() {
        if constexpr (std::is_same_v<CharT, char>) return "false";
        else return L"false";
    }
};

template<typename CharT, typename T>
void to_json(basic_json<CharT>& j, std::vector<T> const& vec) {
    using array_t = typename basic_json<CharT>::array_t;
    array_t arr;
    arr.reserve(vec.size());
    for (auto const& e : vec) {
        arr.emplace_back(e);
    }
    j = std::move(arr);
}

template<typename CharT, typename T>
void from_json(basic_json<CharT> const& j, std::vector<T>& vec) {
    if (!j.is_array()) throw type_error("from_json: expected array");
    vec.clear();
    for (auto const& el : j.template get<typename basic_json<CharT>::array_t>()) {
        vec.push_back(el.template get<T>());
    }
}

template<typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const basic_json<CharT>& j) {
    auto w = os.width();
    os.width(0);
    int indent = (w > 0 ? static_cast<int>(w) : -1);
    os << j.dump(indent);
    return os;
}

template<typename CharT, typename Traits>
std::basic_istream<CharT, Traits>& operator>>(std::basic_istream<CharT, Traits>& is, basic_json<CharT>& j) {
    std::basic_string<CharT> content{
        std::istreambuf_iterator<CharT, Traits>(is),
        std::istreambuf_iterator<CharT, Traits>()
    };
    j = basic_json<CharT>::parse(content);
    return is;
}
} // namespace jsn

using json = jsn::basic_json<char>;
using wjson = jsn::basic_json<wchar_t>;

inline json operator"" _json(const char* s, size_t) {
    return json::parse(s);
}

inline wjson operator"" _wjson(const wchar_t* s, size_t) {
    return wjson::parse(s);
}

#define FE_1(WHAT, X) WHAT(X)
#define FE_2(WHAT, X, ...) WHAT(X), FE_1(WHAT, __VA_ARGS__)
#define FE_3(WHAT, X, ...) WHAT(X), FE_2(WHAT, __VA_ARGS__)
#define FE_4(WHAT, X, ...) WHAT(X), FE_3(WHAT, __VA_ARGS__)
#define FE_5(WHAT, X, ...) WHAT(X), FE_4(WHAT, __VA_ARGS__)
#define FE_6(WHAT, X, ...) WHAT(X), FE_5(WHAT, __VA_ARGS__)
#define FE_7(WHAT, X, ...) WHAT(X), FE_6(WHAT, __VA_ARGS__)
#define FE_8(WHAT, X, ...) WHAT(X), FE_7(WHAT, __VA_ARGS__)

#define GET_FE_MACRO(_1,_2,_3,_4,_5,_6,_7,_8, NAME, ...) NAME
#define FOR_EACH(WHAT, ...) \
    GET_FE_MACRO(__VA_ARGS__, FE_8, FE_7, FE_6, FE_5, FE_4, FE_3, FE_2, FE_1)(WHAT, __VA_ARGS__)

#define KEY_STR(m)                                                      \
[&]() -> typename basic_json<CharT>::string_t {                         \
    if constexpr (std::is_same_v<CharT, char>)                          \
        return typename basic_json<CharT>::string_t{ #m };              \
    else                                                                \
        return typename basic_json<CharT>::string_t{ L###m };           \
}()

#define TO_JSON_PAIR(m) { KEY_STR(m), basic(p.m) }
#define FROM_JSON_ASSIGN(m) p.m = j.at(KEY_STR(m)) .template get<std::decay_t<decltype(p.m)>>()

#define JSON_DEFINE_TYPE(Type, ...)                                     \
namespace jsn {                                                         \
    template<typename CharT>                                            \
    inline void to_json(basic_json<CharT>& j, Type const& p) {          \
        using basic = basic_json<CharT>;                                \
        using string_t = typename basic::string_t;                      \
        using object_t = typename basic::object_t;                      \
        object_t obj = { FOR_EACH(TO_JSON_PAIR, __VA_ARGS__) };         \
        j = std::move(obj);                                             \
    }                                                                   \
                                                                        \
    template<typename CharT>                                            \
    inline void from_json(basic_json<CharT> const& j, Type& p) {        \
        if (!j.is_object())                                             \
            throw type_error("from_json " #Type ": expected object");   \
        FOR_EACH(FROM_JSON_ASSIGN, __VA_ARGS__);                        \
    }                                                                   \
}
