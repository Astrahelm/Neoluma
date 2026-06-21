#pragma once

#include <vector>
#include <map>

#include "Core/Compiler.hpp"

struct PlatformTarget {
    enum class Platform { Windows, Linux, MacOS, iOS, Android, Other };
    Platform platform;
    std::string arch;     // x86_64, arm64, wasm32

    std::string toString() const {
        std::string s = arch + "-";

        switch (platform) {
            case Platform::Windows: s += "windows"; break;
            case Platform::Linux:   s += "linux"; break;
            case Platform::MacOS:   s += "macos"; break;
            case Platform::iOS:     s += "ios"; break;
            case Platform::Android: s += "android"; break;
            default:                s += "unknown"; break;
        }
        
        return s;
    }
};

enum class License { MIT, Apache, GPL2, GPL3, BSD2, BSD3, Boost, CC0, Eclipse, AGPL, LGPL, Mozilla, Unlicense, Custom };

using ProjectSettingValue = std::variant<bool, int64_t, double, std::string>;

// ProjectConfig is a struct that allows me to determine project structure.
struct ProjectConfig {
    std::string name = "Untitled Project";
    std::string version = "1.0.0";
    std::vector<std::string> author = { "Untitled Author" };
    std::vector<PlatformTarget> targets;
    License license = License::MIT;
    OutputType output = OutputType::Executable;
    std::string sourceFolder = "src/";
    std::string buildFolder = "build/";
    std::map<std::string, std::string> dependencies;
    std::map<std::string, std::string> tasks;
    std::map<std::string, std::string> tests;
    std::map<std::string, std::string> languagePacks;

    //CompilerSettings settings;
    std::map<std::string, ProjectSettingValue> compilerSettings;
    std::vector<std::string> filesList; // List of files inside the project to feed to compiler.
    std::string sourcePath; // Absolute path to locate the project
};