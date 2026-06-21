#pragma once
/*
* Localization is an internal Neoluma library for handling localizations of the internal docs / cli / errors / etc.
*/

#include <string>
#include <unordered_map>
#include "../Json/Json.hpp"

namespace Localization {
	extern std::unordered_map<std::string, std::string> localeMap;

	// ==== Internal helpers ====
	std::string detectSystemLanguage(); // runs only once if configs don't exist
	std::unordered_map<std::string, std::string> loadJson(const std::string& filePath);

	// ==== Main functions ====
	void init();
	void pancakeJson(const json::Value& v, const std::string& prefix, std::unordered_map<std::string, std::string>& out);

	std::string translate(const std::string& key);
	std::string translatef(const std::string& key, const std::vector<std::string>& args);
}