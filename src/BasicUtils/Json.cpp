#include "Json.h"
#include "Utils.h"
#include "Logger.h"
#include <sstream>
#include <cwctype>
#include <iomanip>

Json& Json::operator[](const std::wstring& key)
{
    if (auto* object_ptr = std::get_if<Object>(&m_value)) {
        auto it = object_ptr->find(key);
        if (it != object_ptr->end()) {
            return it->second;
        }
        return (*object_ptr)[key];
    }

    m_value = Object();
    return std::get<Object>(m_value)[key];
}

Json& Json::operator[](size_t index)
{
    return at(index);
}

Json& Json::operator=(std::initializer_list<std::pair<std::wstring, Json>> list)
{
    m_value = Object(list.begin(), list.end());
    return *this;
}

Json& Json::operator=(std::initializer_list<Value> list)
{
    m_value = Array(list.begin(), list.end());
    return *this;
}

std::wostream& operator<<(std::wostream& os, const Json& json)
{
    std::wostringstream oss;
    json.dump_impl(oss, 0, 0);
    os << oss.str();
    return os;
}

std::wistream& operator>>(std::wistream& is, Json& json)
{
    json = Json::parse(is);
    return is;
}

bool operator==(const Json& lhs, const Json& rhs)
{
    return lhs.m_value == rhs.m_value;
}

bool operator!=(const Json& lhs, const Json& rhs)
{
    return !(lhs == rhs);
}

Json::Object::iterator Json::begin()
{
    if (is_object()) {
        return std::get<Object>(m_value).begin();
    }
    throw std::runtime_error("Json value is not an object");
}

Json::Object::const_iterator Json::begin() const
{
    return const_cast<Json*>(this)->begin();
}

Json::Object::iterator Json::end()
{
    if (is_object()) {
        return std::get<Object>(m_value).end();
    }
    throw std::runtime_error("Json value is not an object");
}

Json::Object::const_iterator Json::end() const
{
    return const_cast<Json*>(this)->end();
}

Json::Object::iterator Json::find(const std::wstring& key)
{
    if (is_object()) {
        return std::get<Object>(m_value).find(key);
    }
    throw std::runtime_error("Json value is not an object");
}

Json::Object::const_iterator Json::find(const std::wstring& key) const
{
    return const_cast<Json*>(this)->find(key);
}

//Json::Array::iterator Json::begin()
//{
//    if (is_array()) {
//        return std::get<Array>(m_value).begin();
//    }
//    throw std::runtime_error("Json value is not an array");
//}
//
//Json::Array::iterator Json::end()
//{
//    if (is_array()) {
//        return std::get<Array>(m_value).end();
//    }
//    throw std::runtime_error("Json value is not an array");
//}

bool Json::is_null() const
{
    return std::holds_alternative<nullptr_t>(m_value);
}

bool Json::is_integer() const
{
    return std::holds_alternative<int>(m_value);
}

bool Json::is_float() const
{
    return std::holds_alternative<float>(m_value);
}

bool Json::is_double() const
{
    return std::holds_alternative<double>(m_value);
}

bool Json::is_boolean() const
{
    return std::holds_alternative<bool>(m_value);
}

bool Json::is_string() const
{
    return std::holds_alternative<std::wstring>(m_value);
}

bool Json::is_object() const
{
    return std::holds_alternative<Object>(m_value);
}

bool Json::is_array() const
{
    return std::holds_alternative<Array>(m_value);
}

int Json::get_integer() const
{
    if (std::holds_alternative<int>(m_value)) {
        return std::get<int>(m_value);
    }
    Log(L"JSON value is not an integer", LogLevel::Error);
    return 0;
}

float Json::get_float() const
{
    if (std::holds_alternative<float>(m_value)) {
        return std::get<float>(m_value);
    }
    Log(L"JSON value is not an float", LogLevel::Error);
    return 0.0f;
}

double Json::get_double() const
{
    if (std::holds_alternative<double>(m_value)) {
        return std::get<double>(m_value);
    }
    Log(L"JSON value is not a double", LogLevel::Error);
    return 0.0;
}

bool Json::get_boolean() const
{
    if (std::holds_alternative<bool>(m_value)) {
        return std::get<bool>(m_value);
    }
    Log(L"JSON value is not a boolean", LogLevel::Error);
    return false;
}

std::wstring Json::get_string() const
{
    if (std::holds_alternative<std::wstring>(m_value)) {
        return std::get<std::wstring>(m_value);
    }
    Log(L"JSON value is not a string", LogLevel::Error);
    return L"";
}

Json::Object Json::get_object() const
{
    if (std::holds_alternative<Object>(m_value)) {
        return std::get<Object>(m_value);
    }
    Log(L"JSON value is not an object", LogLevel::Error);
    return Object();
}

Json::Array Json::get_array() const
{
    if (std::holds_alternative<Array>(m_value)) {
        return std::get<Array>(m_value);
    }
    Log(L"JSON value is not an array", LogLevel::Error);
    return Array();
}

Json& Json::at(const std::wstring& key)
{
    if (auto* object_ptr = std::get_if<Object>(&m_value)) {
        auto it = object_ptr->find(key);
        if (it != object_ptr->end()) {
            return it->second;
        }
        throw std::out_of_range("Key not found in JSON object");
    }
    throw std::runtime_error("Trying to access key in non-object JSON value");
}

const Json& Json::at(const std::wstring& key) const
{
    return const_cast<Json&>(*this).at(key);
}

Json& Json::at(std::size_t index)
{
    if (auto* array_ptr = std::get_if<Array>(&m_value)) {
        if (index < array_ptr->size()) {
            return (*array_ptr)[index];
        }
        throw std::out_of_range("Index out of range in JSON array");
    }
    throw std::out_of_range("Trying to access index in non-array JSON value");
}

const Json& Json::at(std::size_t index) const
{
    return const_cast<Json&>(*this).at(index);
}

void Json::clear()
{
    if (std::holds_alternative<Object>(m_value)) {
        std::get<Object>(m_value).clear();
    }
    else if (std::holds_alternative<Array>(m_value)) {
        std::get<Array>(m_value).clear();
    }
}

bool Json::empty() const
{
    if (std::holds_alternative<Object>(m_value)) {
        return std::get<Object>(m_value).empty();
    }
    else if (std::holds_alternative<Array>(m_value)) {
        return std::get<Array>(m_value).empty();
    }
    else if (std::holds_alternative<nullptr_t>(m_value)) {
        return true;
    }
    return false;
}

std::size_t Json::size() const
{
    if (std::holds_alternative<Object>(m_value)) {
        return std::get<Object>(m_value).size();
    }
    else if (std::holds_alternative<Array>(m_value)) {
        return std::get<Array>(m_value).size();
    }
    return 0;
}

bool Json::contains(const std::wstring& key) const
{
    if (is_object()) {
        const auto& object = std::get<Object>(m_value);
        return object.find(key) != object.end();
    }
    return false;
}

std::wstring Json::dump(int indent) const
{
    std::wostringstream os;
    dump_impl(os, indent, indent);
    return os.str();
}

void Json::dump_impl(std::wostream& os, int indent, int step) const
{
    std::visit([&os, indent, step](const auto& value) {
        if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Object>) {
            os << (indent == 0 ? L"{" : L"{\n");
            bool first = true;
            for (const auto& [key, val] : value) {
                if (!first) {
                    os << (indent == 0 ? L", " : L",\n");
                }
                first = false;
                os << std::wstring(indent, L' ') << L"\"" << key << L"\": ";
                val.dump_impl(os, indent + step, step);
            }
            os << (indent == 0 ? L"}" : L"\n" + std::wstring(indent - step, L' ') + L"}");
        }
        else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Array>) {
            os << (indent == 0 ? L"[" : L"[\n");
            bool first = true;
            for (const auto& val : value) {
                if (!first) {
                    os << (indent == 0 ? L", " : L",\n");
                }
                first = false;
                if (indent != 0)
                    os << std::wstring(indent, L' ');
                val.dump_impl(os, indent + step, step);
            }
            os << (indent == 0 ? L"]" : L"\n" + std::wstring(indent - step, L' ') + L"]");
        }
        else {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::wstring>)
                os << std::quoted(value);
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, bool>)
                os << (value ? L"true" : L"false");
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::nullptr_t>)
                os << L"null";
            else
                os << value;
        }
        }, m_value);
}

Json Json::parse(const std::wstring& json_text)
{
    std::wistringstream iss(json_text);
    return parse(iss);
}

Json Json::parse(std::wistream& is)
{
    try {
        wchar_t ch;
        is >> ch;
        is.unget();

        if (ch == L'{') {
            return parse_object(is);
        }
        else if (ch == L'[') {
            return parse_array(is);
        }
        else if (ch == L'"') {
            return parse_string(is);
        }
        else if (ch == L'-' || std::iswdigit(ch)) {
            return parse_number(is);
        }
        else if (ch == L't' || ch == L'f') {
            return parse_boolean(is);
        }
        else if (ch == L'n') {
            return parse_null(is);
        }
        else {
            throw std::runtime_error("Invalid JSON");
        }
    }
    catch (const std::exception& e) {
        Log(Utils::FormatString(L"{}", e.what()), LogLevel::Error);
        return Json();
    }
}

Json::Object Json::parse_object(std::wistream& is)
{
    Object result;
    wchar_t ch;
    is >> ch;
    while (is >> ch) {
        if (ch == L'}') {
            return result;
        }
        is.unget();
        std::wstring key = parse_string(is);
        is >> ch;
        if (ch != L':') {
            throw std::runtime_error("Expected ':' in JSON object");
        }
        Json value = parse(is);
        result[key] = value;
        is >> ch;
        if (ch == L'}') {
            return result;
        }
        else if (ch != L',') {
            throw std::runtime_error("Expected ',' or '}' in JSON object");
        }
    }
    throw std::runtime_error("Unexpected end of JSON object");
}

Json::Array Json::parse_array(std::wistream& is)
{
    Array result;
    wchar_t ch;
    is >> ch;
    while (is >> ch) {
        if (ch == L']') {
            return result;
        }
        is.unget();
        Json value = parse(is);
        result.push_back(value);
        is >> ch;
        if (ch == L']') {
            return result;
        }
        else if (ch != L',') {
            throw std::runtime_error("Expected ',' or ']' in JSON array");
        }
    }
    throw std::runtime_error("Unexpected end of JSON array");
}

std::wstring Json::parse_string(std::wistream& is)
{
    std::wstring result;
    wchar_t ch;
    is >> std::ws;
    if (!(is >> ch) || ch != L'"') {
        throw std::runtime_error("Expected opening quote in JSON string");
    }

    while (is.get(ch)) {
        if (ch == L'\\') {
            if (!(is.get(ch))) {
                throw std::runtime_error("Unexpected end of JSON string");
            }
            switch (ch) {
            case L'"': result += L'"'; break;
            case L'\\': result += L'\\'; break;
            case L'/': result += L'/'; break;
            case L'b': result += L'\b'; break;
            case L'f': result += L'\f'; break;
            case L'n': result += L'\n'; break;
            case L'r': result += L'\r'; break;
            case L't': result += L'\t'; break;
            case L'u': {
                std::wstring hex_code;
                for (int i = 0; i < 4; ++i) {
                    if (!(is.get(ch))) {
                        throw std::runtime_error("Incomplete Unicode escape sequence in JSON string");
                    }
                    hex_code += ch;
                }
                wchar_t unicode_char = static_cast<wchar_t>(std::stoi(hex_code, nullptr, 16));
                result += unicode_char;
                break;
            }
            default: throw std::runtime_error("Invalid JSON string escape sequence");
            }
        }
        else if (ch == L'"') {
            break;
        }
        else {
            result += ch;
        }
    }

    if (ch != L'"') {
        throw std::runtime_error("Unexpected end of JSON string");
    }

    return result;
}

Json Json::parse_number(std::wistream& is)
{
    wchar_t ch;
    std::wstring number_str;
    while (is >> ch && (std::iswdigit(ch) || ch == L'.' || ch == L'e' || ch == L'E' || ch == L'+' || ch == L'-')) {
        number_str += ch;
    }
    is.unget();
    std::wistringstream number_iss(number_str);
    double number;
    number_iss >> number;
    if (number_iss.fail() || !number_iss.eof()) {
        throw std::runtime_error("Invalid JSON number");
    }

    if (number_str.find(L'.') != std::wstring::npos || number_str.find(L'e') != std::wstring::npos || number_str.find(L'E') != std::wstring::npos) {
        return number;
    }
    else {
        return static_cast<int>(number);
    }
}

Json Json::parse_boolean(std::wistream& is)
{
    std::wstring boolean_str;
    wchar_t ch;
    for (int i = 0; i < 5; ++i) {
        if (!(is.get(ch))) {
            throw std::runtime_error("Unexpected end of JSON boolean");
        }
        boolean_str += ch;
        if (boolean_str == L"true") {
            return true;
        }
        else if (boolean_str == L"false") {
            return false;
        }
    }
    throw std::runtime_error("Invalid JSON boolean");
}

Json Json::parse_null(std::wistream& is)
{
    std::wstring null_str;
    wchar_t ch;
    for (int i = 0; i < 4; ++i) {
        if (!(is.get(ch))) {
            throw std::runtime_error("Unexpected end of JSON null");
        }
        null_str += ch;
    }
    if (null_str == L"null") {
        return Json();
    }
    else {
        throw std::runtime_error("Invalid JSON null");
    }
}
