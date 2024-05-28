#ifndef JSON_H
#define JSON_H

#include <unordered_map>
#include <vector>
#include <variant>
#include <iostream>
#include <string>

template <typename T, template <typename...> class Template>
struct is_specialization : std::false_type {};

template <template <typename...> class Template, typename... Args>
struct is_specialization<Template<Args...>, Template> : std::true_type {};

template <typename T>
inline constexpr bool is_map_v = is_specialization<std::remove_cvref_t<T>, std::unordered_map>::value;

template <typename T>
inline constexpr bool is_vector_v = is_specialization<std::remove_cvref_t<T>, std::vector>::value;

class Json {
public:
    using Object = std::unordered_map<std::wstring, Json>;
    using Array = std::vector<Json>;
    using Value = std::variant<std::nullptr_t, Object, Array, std::wstring, bool, int, float, double>;

    enum class ValueType { Null, Object, Array, String, Boolean, Integer, Double };

    template <typename T>
        requires std::is_constructible_v<Value, T>
    Json(T&& value) : m_value(std::forward<T>(value)) {}

    template <typename T>
        requires std::is_constructible_v<Value, T>
    Json(const std::vector<T>& list) : m_value(Array(list.begin(), list.end())) {}

    template <typename T>
        requires std::is_constructible_v<Value, T>
    Json(const std::initializer_list<T>& list) : m_value(Array(list.begin(), list.end())) {}

    Json() : m_value(nullptr) {}
    Json(std::initializer_list<std::pair<std::wstring, Json>> list) : m_value(Object(list.begin(), list.end())) {}

    Json& operator[](const std::wstring& key);
    Json& operator[](size_t index);

    Json& operator=(std::initializer_list<std::pair<std::wstring, Json>> list);
    Json& operator=(std::initializer_list<Value> list);

    friend std::wostream& operator<<(std::wostream& os, const Json& json);
    friend std::wistream& operator>>(std::wistream& is, Json& json);
    friend bool operator==(const Json& lhs, const Json& rhs);
    friend bool operator!=(const Json& lhs, const Json& rhs);

    Object::iterator begin();
    Object::const_iterator begin() const;
    
    Object::iterator end();
    Object::const_iterator end() const;
    
    Object::iterator find(const std::wstring& key);
    Object::const_iterator find(const std::wstring& key) const;

    bool is_null() const;
    bool is_integer() const;
    bool is_double() const;
    bool is_boolean() const;
    bool is_string() const;
    bool is_object() const;
    bool is_array() const;

    int get_integer() const;
    double get_double() const;
    bool get_boolean() const;
    std::wstring get_string() const;
    Object get_object() const;
    Array get_array() const;

    template <typename T>
    void get_to(T& value) const 
    {
        if constexpr (std::is_same_v<T, Json>) {
            value = *this;
        }
        else if constexpr (std::is_constructible_v<Value, T>) {
            if (!std::holds_alternative<T>(m_value)) {
                throw std::runtime_error("JSON value cannot be converted to the requested type");
            }
            value = std::get<T>(m_value);
        }
        else if constexpr (is_vector_v<T>) {
            if (!std::holds_alternative<Array>(m_value)) {
                throw std::runtime_error("JSON value is not an array");
            }
            const Array& array = std::get<Array>(m_value);
            value.clear();
            for (const Json& element : array) {
                typename T::value_type element_value;
                element.get_to(element_value);
                value.push_back(element_value);
            }
        }
        else if constexpr (is_map_v<T>) {
            if (!std::holds_alternative<Object>(m_value)) {
                throw std::runtime_error("JSON value is not an object");
            }
            const Object& object = std::get<Object>(m_value);
            value.clear();
            for (const auto& [key, element] : object) {
                typename T::mapped_type element_value;
                element.get_to(element_value);
                value[key] = element_value;
            }
        }
        else {
            throw std::runtime_error("Unsupported type for conversion from JSON");
        }
    }

    Json& at(const std::wstring& key);
    const Json& at(const std::wstring& key) const;

    Json& at(size_t index);
    const Json& at(size_t index) const;

    void clear();
    bool empty() const;
    size_t size() const;
    ValueType type() const;
    
    bool contains(const std::wstring& key) const;
    size_t count(const std::wstring& key) const;
    void erase(const std::wstring& key);

    std::wstring dump(int indent = 0) const;

    static Json parse(const std::wstring& json_text);
    static Json parse(std::wistream& is);

private:
    Value m_value;

    void dump_impl(std::wostream& os, int indent, int step) const;
    static Object parse_object(std::wistream& is);
    static Array parse_array(std::wistream& is);
    static std::wstring parse_string(std::wistream& is);
    static Json parse_number(std::wistream& is);
    static Json parse_boolean(std::wistream& is);
    static Json parse_null(std::wistream& is);
};

#endif // JSON_H