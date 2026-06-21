#include "HelperFunctions.hpp"
#include "Libraries/Color/Color.hpp"
#include <print>
#include <fstream>
#include <sstream>


// Reads the file
String readFile(const String& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::println(std::cerr, "{}[Neoluma/HelperFunctions] Failed to open file: {}", Color::TextHex("#ff5050"), filePath);
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

String normalizePath(String path) {
    size_t pos = 0;
    while ((pos = path.find("\\", pos)) != String::npos) {
        path.replace(pos, 1, "/");
        pos += 1;
    }
    return path;
}