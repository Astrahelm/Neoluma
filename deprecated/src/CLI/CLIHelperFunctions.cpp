#include "CLIHelperFunctions.hpp"

#include "CLI.hpp"
#include "Libraries/Asker/Asker.hpp"
#include "Libraries/Color/Color.hpp"
#include "Libraries/Localization/Localization.hpp"

// ======== Helping functions ========

// Lists authors by comma. If author is only mentioned once, just author name is inputted
std::string listAuthors(const std::vector<std::string>& authors) {
    std::string authorList;
    bool first = true;

    for (const std::string& raw : authors) {
        std::string name = trim(raw);
        if (name.empty()) continue;

        if (!first) authorList += ", ";
        authorList += name;
        first = false;
    }

    return authorList;
}

// Formats the input string to letters and underscores.
std::string formatNameToSnakeCase(const std::string& input) {
    std::string result = input;

    for (char& c : result) {
        if (c == ' ') c = '_';
        else c = std::tolower(static_cast<unsigned char>(c));
    }

    return result;
}

// Returns progress bar for CLI
void showProgressBar(const std::string& stepName, int step, int total) {
    int percentage = (step * 100) / total;
    int hashes = percentage / 5;
    if (percentage != 100) std::println("{} [ {} ({}{}) {}% ] {}", Color::TextHex("#f6ff75"), stepName, std::string(hashes, '#'), std::string(20 - hashes, '_'), percentage, Color::TextHex("#01e0d4"));
    else std::println("{} [ {} ({}{}) {}%! ] {}", Color::TextHex("#75ff87"), stepName, std::string(hashes, '#'), std::string(20 - hashes, '_'), percentage, Color::TextHex("#01e0d4"));
}

// Clears terminal screen
void clearScreen() {
    std::cout << "\033[0m\033[2J\033[H";
}

// -------- stuff required for parseProjectFile
std::string getString(const Toml::TomlTable& table, const std::string& key, const std::string& def) {
    for (const auto& [k, v] : table)
        if (k == key && v.type == Toml::TomlType::String)
            return std::get<std::string>(v.value);
    return def;
}

std::vector<std::string> getStringArray(const Toml::TomlTable& table, const std::string& key) {
    for (const auto& [k, v] : table) {
        if (k == key && v.type == Toml::TomlType::Array) {
            std::vector<std::string> result;
            const auto& arr = std::get<Toml::TomlArray>(v.value);
            for (const auto& item : arr)
                if (item.type == Toml::TomlType::String)
                    result.push_back(std::get<std::string>(item.value));
            return result;
        }
    }
    return {};
}

std::map<std::string, std::string> extractMap(const Toml::TomlTable& root, const std::string& key) {
    std::map<std::string, std::string> result;

    for (const auto& [k, v] : root) {
        if (k == key && v.type == Toml::TomlType::Table) {
            const auto& tbl = std::get<Toml::TomlTable>(v.value);
            for (const auto& [tk, tv] : tbl)
                if (tv.type == Toml::TomlType::String)
                    result[tk] = std::get<std::string>(tv.value);
        }
    }

    return result;
}
// --------

// Argument parsing
CLIArgs parseArgs(int argc, char** argv) {
    CLIArgs args;
    if (argc > 1) args.command = argv[1];

    for (int i = 2; i < argc; i++) {
        std::string token = argv[i];
        if (token.rfind("--", 0) == 0) {
            std::string key = token.substr(2);
            std::string value;

            while (i + 1 < argc && argv[i + 1][0] != '-') {
                if (!value.empty()) value += " ";
                value += argv[++i];
            }
            args.options[key] = value;
        } else args.positional.push_back(token);
    }

    return args;
}

ProjectConfig parseProjectFile(const std::string& file) {
    std::string contents = readFile(file);
    std::istringstream ss(contents);
    Toml::TomlTable root = Toml::parseToml(ss);
    ProjectConfig config;

    // project table parsing
    auto it = std::find_if(root.begin(), root.end(), [](const auto& kv){ return kv.first == "project"; });
    if (it != root.end() && it->second.type == Toml::TomlType::Table) {
        const auto& project = std::get<Toml::TomlTable>(it->second.value);
        config.name = getString(project, "name", config.name);
        config.version = getString(project, "version", config.version);
        config.author = getStringArray(project, "authors");
        config.license = parseLicense(getString(project, "license", ""));
        config.sourceFolder = getString(project, "sourceFolder", config.sourceFolder);
        config.output = parseOutput(getString(project, "output", ""));
        config.buildFolder = getString(project, "buildFolder", config.buildFolder);
    }

    // get configured tasks, dependencies, tests and languagePacks
    config.tasks = extractMap(root, "tasks");
    config.dependencies = extractMap(root, "dependencies");
    config.tests = extractMap(root, "tests");
    config.languagePacks = extractMap(root, "languagePacks");

    // configure source path
    config.sourcePath = std::filesystem::path(file).parent_path().string();

    return config;
}

CompilerSettings parseCompilerSettings(const std::map<std::string, ProjectSettingValue>& map) {
    CompilerSettings config;

    config.verbose = map.contains("verbose") ? std::get<bool>(map.at("verbose")) : config.verbose;
    config.baremetal = map.contains("baremetal") ? std::get<bool>(map.at("baremetal")) : config.baremetal;

    return config;
}

std::string licenseID(License license) {
    switch (license) {
        case License::AGPL: return "agpl";
        case License::Apache: return "apache";
        case License::Boost: return "boost";
        case License::BSD2: return "bsd2";
        case License::BSD3: return "bsd3";
        case License::CC0: return "cc0";
        case License::Eclipse: return "eclipse";
        case License::GPL2: return "gpl2";
        case License::GPL3: return "gpl3";
        case License::LGPL: return "lgpl";
        case License::MIT: return "mit";
        case License::Mozilla: return "mozilla";
        case License::Unlicense: return "unlicense";
        default: return "custom";
    }
}

std::string outputID(OutputType type) {
    switch (type) {
        case OutputType::Executable: return "exe";
        case OutputType::IR: return "ir";
        case OutputType::LLVM_IR: return "llvm_ir";
        case OutputType::Object: return "obj";
        case OutputType::SharedLibrary: return "sharedlib";
        case OutputType::StaticLibrary: return "staticlib";
        default: return "";
    }
}

License parseLicense(std::string license) {
    if (license == "mit") return License::MIT;
    if (license == "apache") return License::Apache;
    if (license == "gpl3") return License::GPL3;
    if (license == "bsd2") return License::BSD2;
    if (license == "bsd3") return License::BSD3;
    if (license == "boost") return License::Boost;
    if (license == "cc0") return License::CC0;
    if (license == "eclipse") return License::Eclipse;
    if (license == "agpl") return License::AGPL;
    if (license == "gpl2") return License::GPL2;
    if (license == "lgpl") return License::LGPL;
    if (license == "mozilla") return License::Mozilla;
    if (license == "unlicense") return License::Unlicense;
    return License::Custom;
}

OutputType parseOutput(std::string outputType) {
    if (outputType == "exe") return OutputType::Executable;
    if (outputType == "ir") return OutputType::IR;
    if (outputType == "llvm_ir") return OutputType::LLVM_IR;
    if (outputType == "obj") return OutputType::Object;
    if (outputType == "sharedlib") return OutputType::SharedLibrary;
    if (outputType == "staticlib") return OutputType::StaticLibrary;
    std::println(std::cerr, "{}[NeolumaCLI/IDtoOutput] {}", Color::TextHex("#ff5050"), Localization::translate("CLI.parseProjectFile.parseOutputError"));
    return OutputType::None;
}