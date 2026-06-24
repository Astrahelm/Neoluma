#include "Localization.hpp"

#include <filesystem>
#include <print>
#include <iostream>
#include <fstream>

#include "Libraries/Toml/Toml.hpp"
#include "Libraries/Paths/Paths.hpp"
#include "Libraries/Utils/Utils.hpp"
#if _WIN32
    #include <Windows.h>
#else
    #include <cstdlib>
#endif

namespace Localization {
    std::unordered_map<std::string, std::string> localeMap;
    std::filesystem::path localeFolder = Paths::dataDir() + "/locales/";

    std::string detectSystemLanguage() {
    #if _WIN32 // i hate microsoft
        ULONG numLangs = 0;
        ULONG bufferLength = 0;

        GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLangs, nullptr, &bufferLength);
        if (bufferLength > 0) {
            std::wstring buffer(bufferLength, L'\0');
            if (GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLangs, buffer.data(), &bufferLength)) {
                std::wstring firstLang = buffer.data(); // first system language

                int len = WideCharToMultiByte(CP_UTF8, 0, firstLang.c_str(), -1, nullptr, 0, nullptr, nullptr);
                std::string out((size_t)len, 0);
                WideCharToMultiByte(CP_UTF8, 0, firstLang.c_str(), -1, out.data(), len, nullptr, nullptr);
                out.resize((size_t)len - 1);

                for (auto& c : out) if (c == '-') c = '_';
                return out;
            }
        }

        // old method fallback just in case
        WCHAR buffer[LOCALE_NAME_MAX_LENGTH];
        int r = GetUserDefaultLocaleName(buffer, LOCALE_NAME_MAX_LENGTH);
        if (r > 0) {
            int len = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, nullptr, 0, nullptr, nullptr);
            std::string out((size_t)len, 0);
            WideCharToMultiByte(CP_UTF8, 0, buffer, -1, out.data(), len, nullptr, nullptr);
            out.resize((size_t)len - 1);

            for (auto& c : out) if (c == '-') c = '_';
            return out;
        }

        return "en_US"; // fallback
    #else
        const char* langPtr = std::getenv("LANG");
        if (!langPtr) return "en_US";

        std::string s = langPtr;

        // LANG often: "en_US.UTF-8"
        if (auto dot = s.find('.'); dot != std::string::npos)
            s = s.substr(0, dot);

        // Sometimes it can be "en_US@modifier"
        if (auto at = s.find('@'); at != std::string::npos)
            s = s.substr(0, at);

        return s;
    #endif
    }

    std::unordered_map<std::string, std::string> loadJson(const std::string& locale) {
        std::unordered_map<std::string, std::string> map;
        // they all must be folders. if there's one guy who messes this up in system files it's their fault
        for (auto& folder : std::filesystem::directory_iterator(localeFolder)){
            if (!folder.is_directory()) {
                std::println(std::cerr, "[Localization] Not a directory: {}", folder.path().string());
                continue;
            }
            std::string name = folder.path().stem().string();
            for (auto& f : std::filesystem::directory_iterator(localeFolder / name))
            {
                if (!f.is_regular_file() || f.path().extension() != ".jsonc") {
                    std::println(std::cerr, "[Localization] Not a locale file found: {}", f.path().string());
                    continue;
                }
                if (f.path().stem().string() != locale) continue;

                json::Value value = json::parseFile(f.path().string());
                pancakeJson(value, name, map);
            }
        }
        return map;
    }

    void pancakeJson(const json::Value& v, const std::string& prefix, std::unordered_map<std::string, std::string>& out) {
        if (v.isObject()) {
            for (const auto& [k, child] : v.asObject()) {
                std::string key = prefix.empty() ? k : (prefix + "." + k);
                pancakeJson(child, key, out);
            }
            return;
        }

        if (!v.isString()) {
            std::println(std::cerr, "[Localization] Non-string value at key: {}", prefix);
            return;
        }

        out[prefix] = v.asString();
    }

    void init() {
        if (!std::filesystem::exists(localeFolder)) {
            std::println(std::cerr, "Neoluma's localization module detected that you have no localization folder. We highly suggest you repairing the installation via your installer. \nNeoluma can't run without the localization.");
            // TODO: Implement a fallback system for people who ever will to delete locale configs.

            return;
        }
        localeMap = loadJson("en_US");
        std::filesystem::path configPath = Paths::userDataDir() + "/config.jsonc";
        if (!std::filesystem::exists(configPath)){
            std::filesystem::create_directory(configPath.parent_path());
            std::string locale = detectSystemLanguage();
            json::Value cfg = json::parse("{//Locale is used to determine your language Neoluma will use.\n//Please do not modify this option unless you're familiar with languages Neoluma supports.\n\"language\": \"" + detectSystemLanguage() + "\"}");
            json::writeFile(configPath.string(), cfg);
        }

        json::Value cfg = json::parseFile(configPath.string());
        std::unordered_map<std::string, std::string> userLanguage = loadJson(cfg["language"].asString());
        for (const auto& [k, v] : userLanguage) localeMap[k] = v;

        //for (const auto& [k, v] : localeMap) std::println(std::cerr, "{}: {}", k, v);
    }

    String translate(const String& key) {
        if (auto it = localeMap.find(key); it != localeMap.end()) return it->second;
        std::println(std::cerr, "[Localization] Couldn't translate key '{}'", key);
        return key;
    }

    // Translate with {} formatting
    String translatef(const String& key, const Array<String>& args)  {
        String result = translate(key);
        return String(result, args);
    }
}