using Neoluma.Core;
using Neoluma.Core.Extras;

namespace Neoluma.CLI;

// ======== Helping functions ========

public static class CLIParser {
    // stuff required for parseProjectFile
    /*
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
     */
    
    public static ProjectConfig parseProjectFile(string file) {
        string contents = File.ReadAllText(file);
        ProjectConfig config = new();
        /*
        std::istringstream ss(contents);
        Toml::TomlTable root = Toml::parseToml(ss);

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
        */
        return config;
    }
}