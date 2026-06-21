#pragma once

#include "Core/Extras/ProjectManager/ProjectManager.hpp"

#include "Libraries/Toml/Toml.hpp"

#include <string>
#include <vector>

struct CLIArgs;
// ======== Helping functions ========

// Lists authors by comma. If author is only mentioned once, just author name is inputted
std::string listAuthors(const std::vector<std::string>& authors);
// Formats the input string to letters and underscores.
std::string formatNameToSnakeCase(const std::string& input);

// Returns progress bar for CLI
void showProgressBar(const std::string& stepName, int step, int total);
// Clears terminal screen
void clearScreen();

// stuff required for parseProjectFile
std::string getString(const Toml::TomlTable& table, const std::string& key, const std::string& def);
std::vector<std::string> getStringArray(const Toml::TomlTable& table, const std::string& key);
std::map<std::string, std::string> extractMap(const Toml::TomlTable& root, const std::string& key);

ProjectConfig parseProjectFile(const std::string& file);
CompilerSettings parseCompilerSettings(const std::map<std::string, ProjectSettingValue>& map);

std::string licenseID(License license);
std::string outputID(OutputType type);
License parseLicense(std::string license);
OutputType parseOutput(std::string outputType);