#include "String.hpp"

String::String() = default;
String::String(const char* value) : str(value) {}
String::String(const std::string& value) : str(value) {}
String::String(std::string_view value) : str(value) {}

String::operator const std::string&() const { return str; }
String::operator std::string&() { return str; }

String& String::operator=(const std::string& value) { str = value; return *this; }
String& String::operator=(std::string&& value) { str = std::move(value); return *this; }
String& String::operator=(const char* value) { str = value; return *this; }
String& String::operator+=(const std::string& value) { str += value; return *this; }
String& String::operator+=(const char* value) { str += value; return *this; }
String& String::operator+=(char value) { str += value; return *this; }
bool String::operator==(const std::string& value) const { return str == value; }
bool String::operator!=(const std::string& value) const { return str != value; }
char& String::operator[](std::size_t index) { return str[index]; }
const char& String::operator[](std::size_t index) const { return str[index]; }

std::size_t String::len() const { return str.length(); }
bool String::empty() const { return str.empty(); }
void String::clear() { str.clear(); }

void String::trim() { str.erase(0, str.find_first_not_of(' ')); }

Array<String> String::split(char delimiter) const {
    Array<String> result;
    String current;
    for (const char c : str) {
        if (c == delimiter) {
            current.trim();
            result.add(current);
            current.clear();
        } else current += c;
    }
    current.trim();
    result.add(current);
    return result;
}
